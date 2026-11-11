#include "AnyUnreal/RHI/Context/ComputeContext.h"
#include "AnyUnreal/RHI/Buffer/Buffer.h"
#include <stdexcept>
#include <utility>

namespace AnyUnreal::RHI
{
    ComputeContext::ComputeContext(CommandBuffer& commands) : commands(commands) {}

    void ComputeContext::requireWritable() const
    {
        if (failed) throw std::runtime_error("Compute context contains a failed command");
    }

    void ComputeContext::RHISetComputePipelineState(FRHIComputePipelineState* pipelineState)
    {
        requireWritable();
        if (!pipelineState || !pipelineState->IsValid()) throw std::invalid_argument("Compute pipeline must be valid");
        if (pipeline.GetReference() == pipelineState) return;
        auto* nextPipeline = static_cast<ComputePipeline*>(pipelineState);
        auto nextParameters = std::make_unique<ComputeParameters>(nextPipeline->GetShader());
        pipeline = nextPipeline;
        parameters = std::move(nextParameters);
    }

    void ComputeContext::RHISetShaderParameters(FRHIComputeShader* computeShader, TConstArrayView<uint8> parametersData, TConstArrayView<FRHIShaderParameter> shaderParameters, TConstArrayView<FRHIShaderParameterResource> resourceParameters, TConstArrayView<FRHIShaderParameterResource> bindlessParameters)
    {
        requireWritable();
        if (!pipeline || computeShader != &pipeline->GetShader()) throw std::invalid_argument("Shader parameters require the active compute shader");
        if (!bindlessParameters.IsEmpty()) throw std::runtime_error("Bindless compute parameters are not implemented");
        try
        {
            parameters->SetConstants(parametersData, shaderParameters);
            parameters->SetResources(resourceParameters);
        }
        catch (...)
        {
            failed = true;
            throw;
        }
    }

    std::unique_ptr<ShaderBindings> ComputeContext::createDispatchBindings() const
    {
        auto dispatchParameters = *parameters;
        const auto staticBindings = staticUniforms.Resolve(pipeline->GetShader());
        dispatchParameters.SetResources({staticBindings.data(), static_cast<int32>(staticBindings.size())});
        auto bindings = dispatchParameters.CreateBindings();
        bindings->Finalize();
        return bindings;
    }

    void ComputeContext::RHIDispatchIndirectComputeShader(FRHIBuffer* argumentBuffer, uint32 argumentOffset)
    {
        requireWritable();
        if (!pipeline || !parameters) throw std::runtime_error("Indirect compute dispatch requires a pipeline");
        if (!argumentBuffer || !EnumHasAnyFlags(argumentBuffer->GetUsage(), EBufferUsageFlags::DrawIndirect)) throw std::invalid_argument("Indirect compute dispatch requires an indirect argument buffer");
        if (argumentOffset % 4 != 0) throw std::invalid_argument("Indirect dispatch argument offset must be aligned to four bytes");
        auto arguments = static_cast<Buffer*>(argumentBuffer)->Capture();
        arguments->GetRange(argumentOffset, 12);
        try
        {
            auto bindings = createDispatchBindings();
            dispatches.push_back({pipeline, std::move(bindings), std::move(arguments)});
            const auto& dispatch = dispatches.back();
            pipeline->RecordIndirectDispatch(commands, *dispatch.bindings, *dispatch.arguments, argumentOffset);
        }
        catch (...)
        {
            failed = true;
            throw;
        }
    }

    void ComputeContext::RHIDispatchComputeShader(uint32 groupX, uint32 groupY, uint32 groupZ)
    {
        requireWritable();
        if (!pipeline || !parameters) throw std::runtime_error("Compute dispatch requires a pipeline");
        try
        {
            auto bindings = createDispatchBindings();
            dispatches.push_back({pipeline, std::move(bindings), {}});
            pipeline->RecordDispatch(commands, *dispatches.back().bindings, groupX, groupY, groupZ);
        }
        catch (...)
        {
            failed = true;
            throw;
        }
    }

    void ComputeContext::RHISetStaticUniformBuffers(const FUniformBufferStaticBindings& uniformBuffers)
    {
        requireWritable();
        staticUniforms.Replace(uniformBuffers);
    }

    void ComputeContext::RHISetStaticUniformBuffer(FUniformBufferStaticSlot slot, FRHIUniformBuffer* uniformBuffer)
    {
        requireWritable();
        staticUniforms.Set(slot, uniformBuffer);
    }
}
