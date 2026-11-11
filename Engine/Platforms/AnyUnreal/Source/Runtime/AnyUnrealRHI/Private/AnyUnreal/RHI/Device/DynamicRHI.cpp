#include "AnyUnreal/RHI/Device/DynamicRHI.h"
#include "AnyUnreal/RHI/Context/GraphicsContext.h"
#include "AnyUnreal/RHI/Buffer/BufferInitializer.h"
#include "AnyUnreal/RHI/Buffer/BufferView.h"
#include "AnyUnreal/RHI/Buffer/StagingBuffer.h"
#include "AnyUnreal/RHI/Buffer/UniformBuffer.h"
#include "AnyUnreal/RHI/Pipeline/ComputePipeline.h"
#include "AnyUnreal/RHI/Pipeline/GraphicsPipeline.h"
#include "AnyUnreal/RHI/Transition/ResourceTransition.h"
#include "AnyUnreal/RHI/Command/CommandList.h"
#include "AnyUnreal/RHI/Command/GpuFence.h"
#include "AnyUnreal/RHI/State/SamplerState.h"
#include "AnyUnreal/RHI/State/RasterizerState.h"
#include "AnyUnreal/RHI/State/DepthStencilState.h"
#include "AnyUnreal/RHI/State/BlendState.h"
#include "AnyUnreal/RHI/State/VertexDeclaration.h"
#include "AnyUnreal/RHI/Texture/TextureInitializer.h"
#include "AnyUnreal/RHI/Texture/TextureView.h"
#include "RHICommandList.h"
#include <stdexcept>
#include <set>
#include <limits>

namespace AnyUnreal::RHI
{
    DynamicRHI::DynamicRHI(Shader::AgcTarget target, std::uint32_t addressHigh) : target(target), addressHigh(addressHigh)
    {
        if (target != Shader::AgcTarget::Windows && target != Shader::AgcTarget::Linux && target != Shader::AgcTarget::Ps5) throw std::invalid_argument("Invalid AGC target");
        if (addressHigh > 0x7fff) throw std::invalid_argument("GPU address aperture exceeds user address space");
    }

    void DynamicRHI::RHIUpdateTexture2D(FRHICommandListBase& commandList, FRHITexture* texture, uint32 mipIndex, const FUpdateTextureRegion2D& region, uint32 sourcePitch, const uint8* sourceData)
    {
        if (mipIndex != 0) throw std::out_of_range("Texture mip does not exist");
        auto destination = ResolveTextureResource(texture).Capture();
        if (!sourceData || region.SrcX < 0 || region.SrcY < 0 || region.Width == 0 || region.Height == 0) throw std::invalid_argument("Invalid texture update source");
        if (region.DestX >= destination->GetWidth() || region.DestY >= destination->GetHeight() || region.Width > destination->GetWidth() - region.DestX || region.Height > destination->GetHeight() - region.DestY) throw std::out_of_range("Texture update exceeds its destination");
        const auto pixelBytes = destination->GetRowBytes() / destination->GetWidth();
        const auto rowBytes = region.Width * pixelBytes;
        const auto columnOffset = static_cast<std::size_t>(region.SrcX) * pixelBytes;
        if (columnOffset > sourcePitch || rowBytes > sourcePitch - columnOffset) throw std::invalid_argument("Texture update source pitch is too small");
        const auto sourceOffset = static_cast<std::size_t>(region.SrcY) * sourcePitch + columnOffset;
        const auto sourceBytes = (region.Height - 1) * static_cast<std::size_t>(sourcePitch) + rowBytes;
        if (sourceOffset > static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max()) - sourceBytes) throw std::overflow_error("Texture update source exceeds pointer range");
        auto upload = std::make_shared<TextureUpload>(region.Width, region.Height, destination->GetFormat(), std::span{reinterpret_cast<const std::byte*>(sourceData) + sourceOffset, sourceBytes}, sourcePitch, addressHigh);
        commandList.EnqueueLambda([destination = std::move(destination), upload = std::move(upload), x = region.DestX, y = region.DestY](FRHICommandListBase& executingList)
        {
            static_cast<GraphicsContext&>(executingList.GetContext()).UploadTexture(destination, upload, x, y);
        });
    }

    uint32 DynamicRHI::RHIComputeMemorySize(FRHITexture* texture)
    {
        const auto size = ResolveTextureResource(texture).GetAllocationSize();
        if (size > std::numeric_limits<uint32>::max()) throw std::overflow_error("Texture allocation exceeds the RHI memory-size range");
        return static_cast<uint32>(size);
    }

    FGraphicsPipelineStateRHIRef DynamicRHI::RHICreateGraphicsPipelineState(const FGraphicsPipelineStateInitializer& initializer)
    {
        return new GraphicsPipeline(initializer);
    }

    FVertexDeclarationRHIRef DynamicRHI::RHICreateVertexDeclaration(const FVertexDeclarationElementList& elements)
    {
        return new VertexDeclaration(elements);
    }

    FRHITextureInitializer DynamicRHI::RHICreateTextureInitializer(FRHICommandListBase& commandList, const FRHITextureCreateDesc& description)
    {
        return CreateTextureInitializer(commandList, description, addressHigh);
    }

    FBlendStateRHIRef DynamicRHI::RHICreateBlendState(const FBlendStateInitializerRHI& initializer)
    {
        return new BlendState(initializer);
    }

    FDepthStencilStateRHIRef DynamicRHI::RHICreateDepthStencilState(const FDepthStencilStateInitializerRHI& initializer)
    {
        return new DepthStencilState(initializer);
    }

    FRasterizerStateRHIRef DynamicRHI::RHICreateRasterizerState(const FRasterizerStateInitializerRHI& initializer)
    {
        return new RasterizerState(initializer);
    }

    FSamplerStateRHIRef DynamicRHI::RHICreateSamplerState(const FSamplerStateInitializerRHI& initializer)
    {
        return new SamplerState(initializer);
    }

    void* DynamicRHI::RHILockBuffer(FRHICommandListBase& commandList, FRHIBuffer* buffer, uint32 offset, uint32 size, EResourceLockMode mode)
    {
        if (!buffer) throw std::invalid_argument("Buffer lock requires a buffer");
        if (mode == RLM_ReadOnly)
        {
            if (!commandList.IsImmediate() || commandList.IsExecuting()) throw std::runtime_error("Synchronous buffer read requires a non-executing immediate command list");
            commandList.GetAsImmediate().SubmitAndBlockUntilGPUIdle();
        }
        return static_cast<Buffer*>(buffer)->Lock(offset, size, mode);
    }

    void DynamicRHI::RHIUnlockBuffer(FRHICommandListBase& commandList, FRHIBuffer* buffer)
    {
        if (!buffer) throw std::invalid_argument("Buffer unlock requires a buffer");
        auto range = static_cast<Buffer*>(buffer)->Unlock();
        if (range.Mode == RLM_ReadOnly) return;
        commandList.EnqueueLambda([buffer = TRefCountPtr<Buffer>(static_cast<Buffer*>(buffer)), range = std::move(range)](FRHICommandListBase& executingList)
        {
            static_cast<ComputeContext&>(executingList.GetContext()).UploadBuffer(buffer.GetReference(), range.Offset, range.Data);
        });
        if (commandList.IsTopOfPipe() || commandList.Bypass()) commandList.RHIThreadFence(true);
    }

    FStagingBufferRHIRef DynamicRHI::RHICreateStagingBuffer()
    {
        return new StagingBuffer();
    }

    void* DynamicRHI::RHILockStagingBuffer(FRHIStagingBuffer* buffer, FRHIGPUFence* fence, uint32 offset, uint32 size)
    {
        if (!buffer) throw std::invalid_argument("Staging buffer is null");
        if (fence && !fence->Poll()) throw std::runtime_error("Staging buffer fence has not completed");
        return buffer->Lock(offset, size);
    }

    void DynamicRHI::RHIUnlockStagingBuffer(FRHIStagingBuffer* buffer)
    {
        if (!buffer) throw std::invalid_argument("Staging buffer is null");
        buffer->Unlock();
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
        if (!resource) throw std::invalid_argument("Shader resource view resource is null");
        if (resource->GetType() == RRT_Buffer) return new BufferShaderResourceView(*static_cast<Buffer*>(resource), description);
        if (resource->GetType() == RRT_Texture) return new TextureShaderResourceView(*static_cast<TextureResource*>(resource), description);
        throw std::invalid_argument("Shader resource view requires a buffer or texture");
    }

    FUnorderedAccessViewRHIRef DynamicRHI::RHICreateUnorderedAccessView(FRHICommandListBase&, FRHIViewableResource* resource, const FRHIViewDesc& description)
    {
        if (!resource) throw std::invalid_argument("Unordered access view resource is null");
        if (resource->GetType() == RRT_Buffer) return new BufferUnorderedAccessView(*static_cast<Buffer*>(resource), description);
        if (resource->GetType() == RRT_Texture) return new TextureUnorderedAccessView(*static_cast<TextureResource*>(resource), description);
        throw std::invalid_argument("Unordered access view requires a buffer or texture");
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
        InitializeResourceTransition(*transition, info, addressHigh);
    }

    void DynamicRHI::RHIReleaseTransition(FRHITransition* transition)
    {
        if (!transition) throw std::invalid_argument("Transition storage is null");
        DestroyResourceTransition(*transition);
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
