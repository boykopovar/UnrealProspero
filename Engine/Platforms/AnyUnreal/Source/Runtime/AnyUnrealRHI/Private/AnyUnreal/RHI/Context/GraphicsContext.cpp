#include "AnyUnreal/RHI/Context/GraphicsContext.h"
#include "AnyUnreal/RHI/Command/Viewport.h"
#include "RHI.h"
#include <stdexcept>
#include <limits>
#include <utility>

namespace AnyUnreal::RHI
{
    GraphicsContext::GraphicsContext(std::size_t initialCommandBytes, std::uint32_t addressHigh) : ComputeContext(initialCommandBytes, addressHigh)
    {
        RecordCommands([](CommandBuffer& commands) { WriteScissor(commands, {0, 0, 32767, 32767}); });
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
