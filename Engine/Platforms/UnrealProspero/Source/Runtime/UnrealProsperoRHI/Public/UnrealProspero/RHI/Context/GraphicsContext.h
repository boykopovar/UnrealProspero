#ifndef UNREALPROSPERO_RHI_GRAPHICS_CONTEXT_H
#define UNREALPROSPERO_RHI_GRAPHICS_CONTEXT_H

#include "UnrealProspero/RHI/Context/ComputeContext.h"
#include "UnrealProspero/RHI/Buffer/VertexStreams.h"
#include "UnrealProspero/RHI/Pipeline/GraphicsPipeline.h"
#include "UnrealProspero/RHI/Pass/RenderPass.h"
#include "UnrealProspero/RHI/Memory/TextureUpload.h"
#include "UnrealProspero/RHI/Memory/VolumeUpload.h"
#include "UnrealProspero/RHI/Query/RenderQuery.h"

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API GraphicsContext : public ComputeContext
    {
    public:
        GraphicsContext(std::size_t initialCommandBytes, std::uint32_t addressHigh);
        using ComputeContext::RHISetShaderParameters;
        void RHISetGraphicsPipelineState(FRHIGraphicsPipelineState* pipelineState, uint32 stencilRef, bool applyAdditionalState) override;
        void RHISetShaderParameters(FRHIGraphicsShader* shader, TConstArrayView<uint8> parametersData, TConstArrayView<FRHIShaderParameter> shaderParameters, TConstArrayView<FRHIShaderParameterResource> resourceParameters, TConstArrayView<FRHIShaderParameterResource> bindlessParameters) override;
        void RHIBeginRenderQuery(FRHIRenderQuery* query) override;
        void RHIEndRenderQuery(FRHIRenderQuery* query) override;
        void RHISetStencilRef(uint32 reference) override;
        void RHISetDepthBounds(float minDepth, float maxDepth) override;
        void RHIBeginRenderPass(const FRHIRenderPassInfo& info, const TCHAR* name) override;
        void RHIEndRenderPass() override;
        void RHICopyTexture(FRHITexture* source, FRHITexture* destination, const FRHICopyTextureInfo& info) override;
        void UploadVolume(std::shared_ptr<const LinearTexture3D> destination, std::shared_ptr<VolumeUpload> upload, uint32 x, uint32 y, uint32 z, uint32 mipLevel);
        void UploadTexture(std::shared_ptr<const LinearTexture2D> destination, std::shared_ptr<TextureUpload> upload, uint32 x, uint32 y, uint32 mipLevel, uint32 arrayLayer = 0);
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
        void ValidateFinalization() const override;

    private:
        struct Draw
        {
            TRefCountPtr<GraphicsPipeline> pipeline;
            std::unique_ptr<ShaderBindings> vertexBindings;
            std::unique_ptr<ShaderBindings> pixelBindings;
            std::unique_ptr<ShaderBindings> geometryBindings;
            std::unique_ptr<MergedDescriptorTables> mergedDescriptors;
            std::unique_ptr<VertexBufferTable> vertexBuffers;
            std::shared_ptr<BufferAllocation> indexBuffer;
            std::shared_ptr<BufferAllocation> arguments;
        };

        void prepareDraw(CommandBuffer& commands, uint32 baseVertexIndex, uint32 firstInstance, std::shared_ptr<BufferAllocation> indexBuffer, std::shared_ptr<BufferAllocation> arguments = {});
        void recordIndirectDraw(CommandBuffer& commands, FRHIBuffer* argumentBuffer, uint32 argumentOffset, std::shared_ptr<BufferAllocation> indices, uint32 indexStride);
        std::vector<std::shared_ptr<OcclusionSnapshot>> querySnapshots;
        std::vector<Draw> draws;
        std::vector<std::shared_ptr<const LinearTexture2D>> copiedTextures;
        std::vector<std::shared_ptr<TextureUpload>> textureUploads;
        std::vector<std::shared_ptr<VolumeUpload>> volumeUploads;
        std::vector<std::shared_ptr<const LinearTexture3D>> copiedVolumes;
        FLinearColor blendFactor{1, 1, 1, 1};
        TRefCountPtr<GraphicsPipeline> pipeline;
        std::unique_ptr<ShaderParameters> vertexParameters;
        std::unique_ptr<ShaderParameters> pixelParameters;
        std::unique_ptr<ShaderParameters> geometryParameters;
        uint32 stencilReference = 0;
        VertexStreams vertexStreams;
        std::vector<std::unique_ptr<RenderPass>> renderPasses;
        RenderPass* activeRenderPass = nullptr;
    };
}

#endif
