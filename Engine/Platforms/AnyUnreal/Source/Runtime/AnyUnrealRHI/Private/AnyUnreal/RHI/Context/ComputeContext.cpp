#include "AnyUnreal/RHI/Context/ComputeContext.h"
#include "AnyUnreal/RHI/Buffer/Buffer.h"
#include "AnyUnreal/RHI/Buffer/BufferView.h"
#include "AnyUnreal/RHI/Command/BufferFill.h"
#include "prx/libSceAgc/DcbState/include/ContextState.hpp"
#include <bit>
#include <stdexcept>
#include <utility>

namespace AnyUnreal::RHI
{
    ComputeContext::ComputeContext(std::size_t initialCommandBytes, std::uint32_t addressHigh) : commandStorage(initialCommandBytes, addressHigh), commands(commandStorage.Get())
    {
        sceAgcDcbContextStateOp(&commands, 0);
    }

    Packet ComputeContext::finalize()
    {
        requireWritable();
        finalized = true;
        return {commands.bottom, static_cast<std::uint32_t>(commands.cursor_up - commands.bottom), 0, {}};
    }

    void ComputeContext::recordTransitions(TArrayView<const FRHITransition*> transitions, bool begin)
    {
        requireWritable();
        try
        {
            for (const auto* transition : transitions)
            {
                if (!transition) throw std::invalid_argument("Resource transition is null");
                auto state = transition->GetPrivateData<TransitionData>()->State;
                if (!state) throw std::invalid_argument("Resource transition has no initialized state");
                retainedTransitions.push_back(state);
                if (begin) state->Begin(commands, GetPipeline());
                else state->End(commands, GetPipeline());
            }
        }
        catch (...)
        {
            failed = true;
            throw;
        }
    }

    void ComputeContext::RHIBeginTransitions(TArrayView<const FRHITransition*> transitions)
    {
        recordTransitions(transitions, true);
    }

    void ComputeContext::RHIEndTransitions(TArrayView<const FRHITransition*> transitions)
    {
        recordTransitions(transitions, false);
    }

    void ComputeContext::clearBufferView(FRHIUnorderedAccessView* view, std::uint32_t value)
    {
        requireWritable();
        if (!view || !view->IsBuffer()) throw std::invalid_argument("UAV clear requires a buffer view");
        const auto range = static_cast<BufferUnorderedAccessView*>(view)->GetRange();
        if (range.empty()) throw std::invalid_argument("UAV clear requires a nonempty view");
        try
        {
            clearedViews.emplace_back(view);
            WriteBufferFill(commands, range.data(), range.size(), value);
        }
        catch (...)
        {
            failed = true;
            throw;
        }
    }

    void ComputeContext::RHIClearUAVUint(FRHIUnorderedAccessView* view, const FUintVector4& values)
    {
        clearBufferView(view, values.X);
    }

    void ComputeContext::RHIClearUAVFloat(FRHIUnorderedAccessView* view, const FVector4f& values)
    {
        clearBufferView(view, std::bit_cast<std::uint32_t>(values.X));
    }

    void ComputeContext::requireWritable() const
    {
        if (failed) throw std::runtime_error("Compute context contains a failed command");
        if (finalized) throw std::runtime_error("Compute context is finalized");
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
