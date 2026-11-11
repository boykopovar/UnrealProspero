#ifndef ANYUNREAL_RHI_GRAPHICS_CONTEXT_H
#define ANYUNREAL_RHI_GRAPHICS_CONTEXT_H

#include "AnyUnreal/RHI/Context/ComputeContext.h"
#include "AnyUnreal/RHI/Buffer/VertexStreams.h"
#include "AnyUnreal/RHI/Pipeline/GraphicsPipeline.h"
#include "AnyUnreal/RHI/Pass/RenderPass.h"
#include "AnyUnreal/RHI/Memory/TextureUpload.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API GraphicsContext : public ComputeContext
    {
    public:
        using ComputeContext::RHISetShaderParameters;
        void RHISetGraphicsPipelineState(FRHIGraphicsPipelineState* pipelineState, uint32 stencilRef, bool applyAdditionalState) override;
        void RHISetShaderParameters(FRHIGraphicsShader* shader, TConstArrayView<uint8> parametersData, TConstArrayView<FRHIShaderParameter> shaderParameters, TConstArrayView<FRHIShaderParameterResource> resourceParameters, TConstArrayView<FRHIShaderParameterResource> bindlessParameters) override;
        void RHISetStencilRef(uint32 reference) override;
        void RHIBeginRenderPass(const FRHIRenderPassInfo& info, const TCHAR* name) override;
        void RHIEndRenderPass() override;
        void RHICopyTexture(FRHITexture* source, FRHITexture* destination, const FRHICopyTextureInfo& info) override;
        void UploadTexture(std::shared_ptr<const LinearTexture2D> destination, std::shared_ptr<TextureUpload> upload, uint32 x, uint32 y);
        void RHISetBlendFactor(const FLinearColor& factor) override;
        void RHIDrawPrimitive(uint32 baseVertexIndex, uint32 primitiveCount, uint32 instanceCount) override;
        void RHIDrawPrimitiveIndirect(FRHIBuffer* argumentBuffer, uint32 argumentOffset) override;
        void RHIDrawIndexedPrimitiveIndirect(FRHIBuffer* indexBuffer, FRHIBuffer* argumentBuffer, uint32 argumentOffset) override;
        void RHIDrawIndexedPrimitive(FRHIBuffer* indexBuffer, int32 baseVertexIndex, uint32 firstInstance, uint32 vertexCount, uint32 startIndex, uint32 primitiveCount, uint32 instanceCount) override;
        void RHISetStreamSource(uint32 streamIndex, FRHIBuffer* vertexBuffer, uint32 offset) override;
        void RHISetViewport(float minX, float minY, float minZ, float maxX, float maxY, float maxZ) override;
        void RHISetMultipleViewports(uint32 count, const FViewportBounds* data) override;
        void RHISetScissorRect(bool enable, uint32 minX, uint32 minY, uint32 maxX, uint32 maxY) override;

    protected:
        GraphicsContext(std::size_t initialCommandBytes, std::uint32_t addressHigh);
        void ValidateFinalization() const override;

    private:
        struct Draw
        {
            TRefCountPtr<GraphicsPipeline> pipeline;
            std::unique_ptr<ShaderBindings> vertexBindings;
            std::unique_ptr<ShaderBindings> pixelBindings;
            std::unique_ptr<VertexBufferTable> vertexBuffers;
            std::shared_ptr<BufferAllocation> indexBuffer;
            std::shared_ptr<BufferAllocation> arguments;
        };

        void prepareDraw(CommandBuffer& commands, uint32 baseVertexIndex, uint32 firstInstance, std::shared_ptr<BufferAllocation> indexBuffer, std::shared_ptr<BufferAllocation> arguments = {});
        void recordIndirectDraw(CommandBuffer& commands, FRHIBuffer* argumentBuffer, uint32 argumentOffset, std::shared_ptr<BufferAllocation> indices, uint32 indexStride);
        std::vector<Draw> draws;
        std::vector<std::shared_ptr<const LinearTexture2D>> copiedTextures;
        std::vector<std::shared_ptr<TextureUpload>> textureUploads;
        FLinearColor blendFactor{1, 1, 1, 1};
        TRefCountPtr<GraphicsPipeline> pipeline;
        std::unique_ptr<ShaderParameters> vertexParameters;
        std::unique_ptr<ShaderParameters> pixelParameters;
        uint32 stencilReference = 0;
        VertexStreams vertexStreams;
        std::vector<std::unique_ptr<RenderPass>> renderPasses;
        RenderPass* activeRenderPass = nullptr;
    };
}

#endif
