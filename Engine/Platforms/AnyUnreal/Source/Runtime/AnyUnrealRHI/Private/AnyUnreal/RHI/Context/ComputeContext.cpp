#include "AnyUnreal/RHI/Context/ComputeContext.h"
#include "AnyUnreal/RHI/Buffer/Buffer.h"
#include "AnyUnreal/RHI/Buffer/BufferView.h"
#include "AnyUnreal/RHI/Buffer/StagingBuffer.h"
#include "AnyUnreal/RHI/Command/PatternFill.h"
#include "AnyUnreal/RHI/Buffer/BufferClearValue.h"
#include "AnyUnreal/RHI/Texture/TextureClearValue.h"
#include "AnyUnreal/RHI/Texture/TextureView.h"
#include "AnyUnreal/RHI/Command/BufferCopy.h"
#include "AnyUnreal/RHI/Command/MemoryAcquire.h"
#include "AnyUnreal/RHI/Command/GpuFence.h"
#include "prx/libSceAgc/DcbState/include/ContextState.hpp"
#include "prx/libSceAgc/DcbState/include/Marker.hpp"
#include "Containers/StringConv.h"
#include "RHICommandList.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace AnyUnreal::RHI
{
#if WITH_RHI_BREADCRUMBS
    void ComputeContext::RHIBeginBreadcrumbGPU(FRHIBreadcrumbNode* breadcrumb)
    {
        RecordCommands([&](CommandBuffer& buffer)
        {
            if (!breadcrumb) throw std::invalid_argument("GPU breadcrumb is null");
            if (!ShouldEmitBreadcrumbs()) return;
            const auto path = breadcrumb->GetFullPath();
            const auto text = StringCast<UTF8CHAR>(*path);
            if (text.Length() >= 65536) throw std::length_error("GPU breadcrumb exceeds the AGC marker size");
            sceAgcDcbPushMarker(&buffer, reinterpret_cast<const char*>(text.Get()), breadcrumb->Data.Color.ToPackedARGB());
        });
    }

    void ComputeContext::RHIEndBreadcrumbGPU(FRHIBreadcrumbNode* breadcrumb)
    {
        RecordCommands([&](CommandBuffer& buffer)
        {
            if (!breadcrumb) throw std::invalid_argument("GPU breadcrumb is null");
            if (ShouldEmitBreadcrumbs()) sceAgcDcbPopMarker(&buffer);
        });
    }
#endif

    ComputeContext::ComputeContext(std::size_t initialCommandBytes, std::uint32_t addressHigh) : commandStorage(initialCommandBytes, addressHigh), completion(addressHigh), commands(commandStorage.Get()), addressHigh(addressHigh)
    {
        sceAgcDcbContextStateOp(&commands, 0);
    }

    Packet ComputeContext::finalize()
    {
        requireWritable();
        ValidateFinalization();
        finalized = true;
        completion.Write(commands);
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

    template<typename TValue>
    void ComputeContext::clearView(FRHIUnorderedAccessView* view, const std::array<TValue, 4>& values)
    {
        requireWritable();
        if (!view) throw std::invalid_argument("UAV clear requires a view");
        std::shared_ptr<const BufferAllocation> storage;
        std::span<std::byte> range;
        std::array<std::uint32_t, 4> pattern;
        std::size_t words;
        if (view->IsBuffer())
        {
            const auto snapshot = static_cast<BufferUnorderedAccessView*>(view)->Capture();
            storage = snapshot.CaptureStorage();
            range = snapshot.GetRange();
            pattern = EncodeBufferClear(snapshot.GetFormat(), values);
            words = snapshot.GetFormat() == BufferFormat::Untyped ? 1u : GetBufferElementSize(snapshot.GetFormat()) / 4;
        }
        else
        {
            const auto& texture = *static_cast<TextureUnorderedAccessView*>(view);
            storage = texture.CaptureStorage();
            if (!storage) throw std::invalid_argument("Texture UAV clear requires allocated storage");
            range = texture.GetRange();
            pattern = EncodeTextureClear(texture.GetFormat(), values);
            words = pattern.size();
        }
        if (range.empty()) throw std::invalid_argument("UAV clear requires a nonempty view");
        try
        {
            clearedResources.push_back(std::move(storage));
            WritePatternFill(commands, range.data(), range.size(), std::span(pattern).first(words));
        }
        catch (...)
        {
            failed = true;
            throw;
        }
    }

    void ComputeContext::RHIClearUAVUint(FRHIUnorderedAccessView* view, const FUintVector4& values)
    {
        clearView(view, std::array<std::uint32_t, 4>{values.X, values.Y, values.Z, values.W});
    }

    void ComputeContext::RHIClearUAVFloat(FRHIUnorderedAccessView* view, const FVector4f& values)
    {
        clearView(view, std::array<float, 4>{values.X, values.Y, values.Z, values.W});
    }

    void ComputeContext::RHICopyBufferRegion(FRHIBuffer* destination, uint64 destinationOffset, FRHIBuffer* source, uint64 sourceOffset, uint64 size)
    {
        requireWritable();
        if (!destination || !source) throw std::invalid_argument("Buffer copy requires source and destination buffers");
        auto destinationStorage = static_cast<Buffer*>(destination)->Capture();
        auto sourceStorage = static_cast<Buffer*>(source)->Capture();
        const auto destinationRange = destinationStorage->GetRange(destinationOffset, size);
        const auto sourceRange = sourceStorage->GetRange(sourceOffset, size);
        if (destinationStorage == sourceStorage && destinationOffset < sourceOffset + size && sourceOffset < destinationOffset + size) throw std::invalid_argument("Buffer copy ranges overlap");
        try
        {
            copiedBuffers.reserve(copiedBuffers.size() + 2);
            copiedBuffers.push_back(std::move(destinationStorage));
            copiedBuffers.push_back(std::move(sourceStorage));
            const GpuMemoryRange ranges[]{{destinationRange.data(), destinationRange.size()}, {sourceRange.data(), sourceRange.size()}};
            WriteMemoryAcquire(commands, ranges);
            WriteBufferCopy(commands, destinationRange.data(), sourceRange.data(), size);
        }
        catch (...)
        {
            failed = true;
            throw;
        }
    }

    void ComputeContext::UploadBuffer(FRHIBuffer* destination, std::size_t offset, std::span<const std::byte> source)
    {
        requireWritable();
        if (!destination) throw std::invalid_argument("Buffer upload destination is null");
        auto storage = static_cast<Buffer*>(destination)->Capture();
        const auto range = storage->GetRange(offset, source.size());
        try
        {
            auto upload = std::make_shared<Upload>(source, addressHigh);
            copiedBuffers.push_back(std::move(storage));
            uploads.push_back(upload);
            upload->Record(commands, range);
        }
        catch (...)
        {
            failed = true;
            throw;
        }
    }

    void ComputeContext::RHICopyToStagingBuffer(FRHIBuffer* source, FRHIStagingBuffer* destination, uint32 offset, uint32 size)
    {
        requireWritable();
        if (!source || !destination) throw std::invalid_argument("Staging copy requires source and destination buffers");
        auto storage = static_cast<Buffer*>(source)->Capture();
        const auto range = storage->GetRange(offset, size);
        try
        {
            auto readback = std::make_shared<Readback>(size, addressHigh);
            copiedBuffers.push_back(std::move(storage));
            readbacks.push_back(readback);
            static_cast<StagingBuffer*>(destination)->Record(commands, readback, range);
        }
        catch (...)
        {
            failed = true;
            throw;
        }
    }

    void ComputeContext::RHIWriteGPUFence(FRHIGPUFence* fence)
    {
        requireWritable();
        if (!fence) throw std::invalid_argument("GPU fence is null");
        try
        {
            auto marker = std::make_shared<Completion>(addressHigh);
            fenceMarkers.push_back(marker);
            static_cast<GpuFence*>(fence)->Write(commands, marker);
        }
        catch (...)
        {
            failed = true;
            throw;
        }
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
        auto nextParameters = std::make_unique<ShaderParameters>(nextPipeline->GetShader());
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

    void ComputeContext::ApplyStaticUniformBuffers(ShaderParameters& shaderParameters) const
    {
        const auto staticBindings = staticUniforms.Resolve(shaderParameters.GetShader());
        shaderParameters.SetResources({staticBindings.data(), static_cast<int32>(staticBindings.size())});
    }

    std::unique_ptr<ShaderBindings> ComputeContext::createDispatchBindings() const
    {
        auto dispatchParameters = *parameters;
        ApplyStaticUniformBuffers(dispatchParameters);
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
