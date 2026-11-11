#include "AnyUnreal/RHI/Device/DynamicRHI.h"
#include "AnyUnreal/RHI/Buffer/BufferInitializer.h"
#include "AnyUnreal/RHI/Buffer/BufferView.h"
#include "AnyUnreal/RHI/Buffer/UniformBuffer.h"
#include "AnyUnreal/RHI/Pipeline/ComputePipeline.h"
#include "AnyUnreal/RHI/Transition/BufferTransition.h"
#include "AnyUnreal/RHI/Command/CommandList.h"
#include "AnyUnreal/RHI/Command/GpuFence.h"
#include <stdexcept>
#include <set>

namespace AnyUnreal::RHI
{
    DynamicRHI::DynamicRHI(Shader::AgcTarget target, std::uint32_t addressHigh) : target(target), addressHigh(addressHigh)
    {
        if (target != Shader::AgcTarget::Windows && target != Shader::AgcTarget::Linux && target != Shader::AgcTarget::Ps5) throw std::invalid_argument("Invalid AGC target");
        if (addressHigh > 0x7fff) throw std::invalid_argument("GPU address aperture exceeds user address space");
    }

    FGPUFenceRHIRef DynamicRHI::RHICreateGPUFence(const FName& name)
    {
        return new GpuFence(name);
    }

    FVertexShaderRHIRef DynamicRHI::RHICreateVertexShader(const FRHICreateShaderDesc& description)
    {
        return new VertexShader(description.Code, FShaderHash{}, target);
    }

    FPixelShaderRHIRef DynamicRHI::RHICreatePixelShader(const FRHICreateShaderDesc& description)
    {
        return new PixelShader(description.Code, FShaderHash{}, target);
    }

    FComputeShaderRHIRef DynamicRHI::RHICreateComputeShader(const FRHICreateShaderDesc& description)
    {
        return new ComputeShader(description.Code, FShaderHash{}, target);
    }

    FComputePipelineStateRHIRef DynamicRHI::RHICreateComputePipelineState(const FComputePipelineStateInitializer& initializer)
    {
        if (!initializer.ComputeShader) throw std::invalid_argument("Compute pipeline requires a shader");
        if (!initializer.SpecializationConstants.IsEmpty() || initializer.ComputeShader->HasSpecializationConstants()) throw std::runtime_error("Shader specialization constants are not implemented");
        return new ComputePipeline(*static_cast<ComputeShader*>(initializer.ComputeShader));
    }

    FUniformBufferRHIRef DynamicRHI::RHICreateUniformBuffer(const void* contents, const FRHIUniformBufferLayout* layout, EUniformBufferUsage usage, EUniformBufferValidation validation)
    {
        return new UniformBuffer(layout, contents, usage, validation, addressHigh);
    }

    FRHIBufferInitializer DynamicRHI::RHICreateBufferInitializer(FRHICommandListBase& commandList, const FRHIBufferCreateDesc& description)
    {
        return CreateBufferInitializer(commandList, description, addressHigh);
    }

    void DynamicRHI::RHIUpdateUniformBuffer(FRHICommandListBase& commandList, FRHIUniformBuffer* buffer, const void* contents)
    {
        if (!buffer) throw std::invalid_argument("Uniform buffer is null");
        static_cast<UniformBuffer*>(buffer)->Update(commandList, contents);
    }

    FShaderResourceViewRHIRef DynamicRHI::RHICreateShaderResourceView(FRHICommandListBase&, FRHIViewableResource* resource, const FRHIViewDesc& description)
    {
        if (!resource || resource->GetType() != RRT_Buffer) throw std::runtime_error("Shader resource views require a buffer; texture views are not implemented");
        return new BufferShaderResourceView(*static_cast<Buffer*>(resource), description);
    }

    FUnorderedAccessViewRHIRef DynamicRHI::RHICreateUnorderedAccessView(FRHICommandListBase&, FRHIViewableResource* resource, const FRHIViewDesc& description)
    {
        if (!resource || resource->GetType() != RRT_Buffer) throw std::runtime_error("Unordered access views require a buffer; texture views are not implemented");
        return new BufferUnorderedAccessView(*static_cast<Buffer*>(resource), description);
    }

    void DynamicRHI::ConfigureTransitionStorage()
    {
        GRHITransitionPrivateData_SizeInBytes = sizeof(TransitionData);
        GRHITransitionPrivateData_AlignInBytes = alignof(TransitionData);
    }

    void DynamicRHI::RHICreateTransition(FRHITransition* transition, const FRHITransitionCreateInfo& info)
    {
        if (!transition) throw std::invalid_argument("Transition storage is null");
        if (GRHITransitionPrivateData_SizeInBytes != sizeof(TransitionData) || GRHITransitionPrivateData_AlignInBytes != alignof(TransitionData)) throw std::runtime_error("Transition storage is not configured");
        InitializeBufferTransition(*transition, info, addressHigh);
    }

    void DynamicRHI::RHIReleaseTransition(FRHITransition* transition)
    {
        if (!transition) throw std::invalid_argument("Transition storage is null");
        DestroyBufferTransition(*transition);
    }

    void DynamicRHI::RHISubmitCommandLists(FRHISubmitCommandListsArgs&& args)
    {
        if (submissionFailed) throw std::runtime_error("GPU submission queue has failed");
        std::set<IRHIPlatformCommandList*> uniqueLists;
        for (const auto& list : pendingCommandLists) uniqueLists.insert(list.get());
        for (auto* list : args.CommandLists)
        {
            if (!list || !uniqueLists.insert(list).second) throw std::invalid_argument("Command list submission contains a null or duplicate list");
        }
        const auto first = pendingCommandLists.size();
        pendingCommandLists.reserve(first + args.CommandLists.Num());
        for (auto* list : args.CommandLists) pendingCommandLists.emplace_back(static_cast<CommandList*>(list));
        args.CommandLists.Reset();
        try
        {
            for (auto index = first; index < pendingCommandLists.size(); ++index) pendingCommandLists[index]->Submit();
        }
        catch (...)
        {
            submissionFailed = true;
            throw;
        }
    }

    void DynamicRHI::RHIProcessDeleteQueue()
    {
        std::erase_if(pendingCommandLists, [](const auto& list) { return list->IsComplete(); });
    }

    void DynamicRHI::RHIBlockUntilGPUIdle()
    {
        if (submissionFailed) throw std::runtime_error("GPU submission queue has failed");
        for (const auto& list : pendingCommandLists) list->Wait(10000000);
        pendingCommandLists.clear();
    }

    FDynamicRHI::FRHIFinalizeContextsResult DynamicRHI::RHIFinalizeContexts(FRHIFinalizeContextsArgs&& args)
    {
        if (args.UploadContext || args.Contexts[ERHIPipeline::AsyncCompute]) throw std::runtime_error("Upload and async compute context finalization is not implemented");
        FRHIFinalizeContextsResult result;
        if (auto* context = args.Contexts[ERHIPipeline::Graphics])
        {
            auto owned = std::unique_ptr<ComputeContext>(static_cast<ComputeContext*>(context));
            result.CommandLists[ERHIPipeline::Graphics] = std::make_unique<CommandList>(std::move(owned)).release();
        }
        return result;
    }
}
