#include "AnyUnreal/RHI/Context/GraphicsContext.h"
#include "AnyUnreal/RHI/Command/Viewport.h"
#include "RHI.h"
#include <stdexcept>
#include <limits>
#include <utility>
#include <cmath>
#include "prx/libSceAgc/DcbDraw/include/DrawIndexed.hpp"
#include "prx/libSceAgc/DcbDraw/include/Instancing.hpp"

namespace AnyUnreal::RHI
{
    GraphicsContext::GraphicsContext(std::size_t initialCommandBytes, std::uint32_t addressHigh) : ComputeContext(initialCommandBytes, addressHigh)
    {
        RecordCommands([](CommandBuffer& commands) { WriteScissor(commands, {0, 0, 32767, 32767}); });
    }

    void GraphicsContext::RHISetBlendFactor(const FLinearColor& factor)
    {
        RecordCommands([&](CommandBuffer&)
        {
            if (!std::isfinite(factor.R) || !std::isfinite(factor.G) || !std::isfinite(factor.B) || !std::isfinite(factor.A)) throw std::invalid_argument("Blend factor must be finite");
            blendFactor = factor;
        });
    }

    void GraphicsContext::RHIDrawPrimitive(uint32 baseVertexIndex, uint32 primitiveCount, uint32 instanceCount)
    {
        RecordCommands([&](CommandBuffer& commands)
        {
            if (!pipeline || !activeRenderPass) throw std::runtime_error("Draw requires a graphics pipeline and render pass");
            activeRenderPass->ValidatePipeline(pipeline->GetDescription());
            const auto vertexCount = pipeline->GetVertexCount(primitiveCount);
            if (vertexCount == 0 || instanceCount == 0) return;
            if (baseVertexIndex > std::numeric_limits<uint32>::max() - (vertexCount - 1)) throw std::overflow_error("Draw vertex index exceeds the hardware field");
            const auto& layout = pipeline->GetShaders().GetVertexShader().GetCode().GetUserData();
            const auto* declaration = pipeline->GetVertexDeclaration();
            if (declaration)
            {
                for (const auto& element : declaration->GetElements())
                {
                    const auto index = layout.UsesVertexBuffersPerAttribute() ? element.AttributeIndex : element.StreamIndex;
                    if (index >= 32 || (layout.GetVertexBufferUsageMask() & (uint32{1} << index)) == 0) continue;
                    const auto snapshot = vertexStreams.Capture(element.StreamIndex);
                    const std::uint64_t lastIndex = element.bUseInstanceIndex ? instanceCount - 1 : static_cast<std::uint64_t>(baseVertexIndex) + vertexCount - 1;
                    const auto end = snapshot.Offset + lastIndex * element.Stride + element.Offset + VertexDeclaration::GetElementSize(element.Type);
                    if (end > snapshot.Storage->Size()) throw std::out_of_range("Draw vertex fetch exceeds its buffer");
                }
            }
            auto vertexBuffers = declaration ? vertexStreams.CaptureTable(*declaration, layout) : std::make_unique<VertexBufferTable>(0, std::span<const VertexBufferBinding>{}, layout.GetAddressHigh());
            auto vertexBindings = vertexParameters->CreateBindings();
            auto pixelBindings = pixelParameters ? pixelParameters->CreateBindings() : nullptr;
            draws.push_back({pipeline, std::move(vertexBindings), std::move(pixelBindings), std::move(vertexBuffers)});
            const auto& draw = draws.back();
            Shader::UserDataInputs inputs;
            inputs.BaseVertex = baseVertexIndex;
            draw.pipeline->GetShaders().Record(commands, *draw.vertexBindings, draw.pixelBindings.get(), *draw.vertexBuffers, inputs);
            draw.pipeline->RecordColorState(commands, stencilReference, blendFactor);
            sceAgcDcbSetNumInstances(&commands, instanceCount);
            sceAgcDcbDrawIndexAuto(&commands, vertexCount, 0);
        });
    }

    void GraphicsContext::ValidateFinalization() const
    {
        if (activeRenderPass) throw std::runtime_error("Cannot finalize commands inside a render pass");
    }

    void GraphicsContext::RHIBeginRenderPass(const FRHIRenderPassInfo& info, const TCHAR*)
    {
        RecordCommands([&](CommandBuffer& commands)
        {
            if (activeRenderPass) throw std::runtime_error("A render pass is already active");
            auto pass = std::make_unique<RenderPass>(info, GetAddressHigh());
            renderPasses.push_back(std::move(pass));
            activeRenderPass = renderPasses.back().get();
            activeRenderPass->Begin(commands);
        });
    }

    void GraphicsContext::RHIEndRenderPass()
    {
        RecordCommands([&](CommandBuffer& commands)
        {
            if (!activeRenderPass) throw std::runtime_error("No render pass is active");
            activeRenderPass->End(commands);
            activeRenderPass = nullptr;
        });
    }

    void GraphicsContext::RHISetGraphicsPipelineState(FRHIGraphicsPipelineState* pipelineState, uint32 stencilReference, bool applyAdditionalState)
    {
        RecordCommands([&](CommandBuffer&)
        {
            if (!pipelineState) throw std::invalid_argument("Graphics pipeline is null");
            if (stencilReference > std::numeric_limits<uint8>::max()) throw std::out_of_range("Stencil reference exceeds eight bits");
            auto* nextPipeline = static_cast<GraphicsPipeline*>(pipelineState);
            auto& vertex = nextPipeline->GetShaders().GetVertexShader();
            auto* pixel = nextPipeline->GetShaders().GetPixelShader();
            auto nextVertex = vertexParameters && &vertexParameters->GetShader() == &vertex ? std::make_unique<ShaderParameters>(*vertexParameters) : std::make_unique<ShaderParameters>(vertex);
            std::unique_ptr<ShaderParameters> nextPixel;
            if (pixel) nextPixel = pixelParameters && &pixelParameters->GetShader() == pixel ? std::make_unique<ShaderParameters>(*pixelParameters) : std::make_unique<ShaderParameters>(*pixel);
            if (applyAdditionalState)
            {
                ApplyStaticUniformBuffers(*nextVertex);
                if (nextPixel) ApplyStaticUniformBuffers(*nextPixel);
            }
            pipeline = nextPipeline;
            vertexParameters = std::move(nextVertex);
            pixelParameters = std::move(nextPixel);
            this->stencilReference = stencilReference;
        });
    }

    void GraphicsContext::RHISetShaderParameters(FRHIGraphicsShader* shader, TConstArrayView<uint8> parametersData, TConstArrayView<FRHIShaderParameter> shaderParameters, TConstArrayView<FRHIShaderParameterResource> resourceParameters, TConstArrayView<FRHIShaderParameterResource> bindlessParameters)
    {
        RecordCommands([&](CommandBuffer&)
        {
            if (!shader || !pipeline) throw std::invalid_argument("Graphics parameters require an active pipeline and shader");
            if (!bindlessParameters.IsEmpty()) throw std::runtime_error("Bindless graphics parameters are not implemented");
            ShaderParameters* parameters = nullptr;
            if (vertexParameters && &vertexParameters->GetShader() == shader) parameters = vertexParameters.get();
            else if (pixelParameters && &pixelParameters->GetShader() == shader) parameters = pixelParameters.get();
            if (!parameters) throw std::invalid_argument("Graphics parameters require a shader from the active pipeline");
            parameters->SetConstants(parametersData, shaderParameters);
            parameters->SetResources(resourceParameters);
        });
    }

    void GraphicsContext::RHISetStencilRef(uint32 reference)
    {
        RecordCommands([&](CommandBuffer&)
        {
            if (reference > std::numeric_limits<uint8>::max()) throw std::out_of_range("Stencil reference exceeds eight bits");
            stencilReference = reference;
        });
    }

    void GraphicsContext::RHISetStreamSource(uint32 streamIndex, FRHIBuffer* vertexBuffer, uint32 offset)
    {
        RecordCommands([&](CommandBuffer&) { vertexStreams.Set(streamIndex, vertexBuffer, offset); });
    }

    void GraphicsContext::RHISetViewport(float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
    {
        RecordCommands([&](CommandBuffer& commands)
        {
            WriteViewport(commands, 0, {minX, minY, minZ, maxX, maxY, maxZ});
            WriteViewportScissor(commands, 0, {0, 0, 32767, 32767});
        });
    }

    void GraphicsContext::RHISetMultipleViewports(uint32 count, const FViewportBounds* data)
    {
        RecordCommands([&](CommandBuffer& commands)
        {
            if (!data || count == 0 || count > 16) throw std::invalid_argument("Multiple viewports require one to sixteen bounds");
            for (uint32 index = 0; index < count; ++index)
            {
                const auto& viewport = data[index];
                if (viewport.Width < 0 || viewport.Height < 0) throw std::out_of_range("Viewport dimensions must be nonnegative");
                WriteViewport(commands, index, {viewport.TopLeftX, viewport.TopLeftY, viewport.MinDepth, viewport.TopLeftX + viewport.Width, viewport.TopLeftY + viewport.Height, viewport.MaxDepth});
                WriteViewportScissor(commands, index, {0, 0, 32767, 32767});
            }
        });
    }

    void GraphicsContext::RHISetScissorRect(bool enable, uint32 minX, uint32 minY, uint32 maxX, uint32 maxY)
    {
        RecordCommands([&](CommandBuffer& commands)
        {
            WriteScissor(commands, enable ? ScissorRect{minX, minY, maxX, maxY} : ScissorRect{0, 0, 32767, 32767});
        });
    }
}
