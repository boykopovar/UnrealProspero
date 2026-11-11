#ifndef ANYUNREAL_RHI_GRAPHICS_CONTEXT_H
#define ANYUNREAL_RHI_GRAPHICS_CONTEXT_H

#include "AnyUnreal/RHI/Context/ComputeContext.h"
#include "AnyUnreal/RHI/Buffer/VertexStreams.h"
#include "AnyUnreal/RHI/Pipeline/GraphicsPipeline.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API GraphicsContext : public ComputeContext
    {
    public:
        using ComputeContext::RHISetShaderParameters;
        void RHISetGraphicsPipelineState(FRHIGraphicsPipelineState* pipelineState, uint32 stencilReference, bool applyAdditionalState) override;
        void RHISetShaderParameters(FRHIGraphicsShader* shader, TConstArrayView<uint8> parametersData, TConstArrayView<FRHIShaderParameter> shaderParameters, TConstArrayView<FRHIShaderParameterResource> resourceParameters, TConstArrayView<FRHIShaderParameterResource> bindlessParameters) override;
        void RHISetStencilRef(uint32 reference) override;
        void RHISetStreamSource(uint32 streamIndex, FRHIBuffer* vertexBuffer, uint32 offset) override;
        void RHISetViewport(float minX, float minY, float minZ, float maxX, float maxY, float maxZ) override;
        void RHISetMultipleViewports(uint32 count, const FViewportBounds* data) override;
        void RHISetScissorRect(bool enable, uint32 minX, uint32 minY, uint32 maxX, uint32 maxY) override;

    protected:
        GraphicsContext(std::size_t initialCommandBytes, std::uint32_t addressHigh);

    private:
        TRefCountPtr<GraphicsPipeline> pipeline;
        std::unique_ptr<ShaderParameters> vertexParameters;
        std::unique_ptr<ShaderParameters> pixelParameters;
        uint32 stencilReference = 0;
        VertexStreams vertexStreams;
    };
}

#endif
