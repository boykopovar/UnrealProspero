#ifndef ANYUNREAL_RHI_DYNAMIC_RHI_H
#define ANYUNREAL_RHI_DYNAMIC_RHI_H

#include "AnyUnreal/Shader/ShaderBinary.h"
#include "AnyUnreal/RHI/Command/CommandList.h"
#include <DynamicRHI.h>
#include <vector>

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API DynamicRHI : public FDynamicRHI
    {
    public:
        void RHIUpdateTexture2D(FRHICommandListBase& commandList, FRHITexture* texture, uint32 mipIndex, const FUpdateTextureRegion2D& region, uint32 sourcePitch, const uint8* sourceData) override;
        FRHILockTextureResult RHILockTexture(FRHICommandListImmediate& commandList, const FRHILockTextureArgs& arguments) override;
        void RHIUnlockTexture(FRHICommandListImmediate& commandList, const FRHILockTextureArgs& arguments) override;
        FRHICalcTextureSizeResult RHICalcTexturePlatformSize(const FRHITextureDesc& description, uint32 firstMipIndex) override;
        void RHIReadSurfaceData(FRHITexture* texture, FIntRect rectangle, TArray<FColor>& output, FReadSurfaceDataFlags flags) override;
        void RHIReadSurfaceData(FRHITexture* texture, FIntRect rectangle, TArray<FLinearColor>& output, FReadSurfaceDataFlags flags) override;
        void RHIMapStagingSurface(FRHITexture* texture, FRHIGPUFence* fence, void*& data, int32& width, int32& height, uint32 gpuIndex = 0) override;
        void RHIUnmapStagingSurface(FRHITexture* texture, uint32 gpuIndex = 0) override;
        FViewportRHIRef RHICreateViewport(void* windowHandle, uint32 width, uint32 height, bool fullscreen, EPixelFormat format) override;
        void RHIResizeViewport(FRHIViewport* viewport, uint32 width, uint32 height, bool fullscreen, EPixelFormat format) override;
        FTextureRHIRef RHIGetViewportBackBuffer(FRHIViewport* viewport) override;
        void RHIEndDrawingViewport(FRHICommandListImmediate& commandList, FRHIViewport* viewport, const FRHIPresentArgs& arguments) override;
        uint32 RHIComputeMemorySize(FRHITexture* texture) override;
        FVertexDeclarationRHIRef RHICreateVertexDeclaration(const FVertexDeclarationElementList& elements) override;
        FVertexShaderRHIRef RHICreateVertexShader(const FRHICreateShaderDesc& description) override;
        FPixelShaderRHIRef RHICreatePixelShader(const FRHICreateShaderDesc& description) override;
        FComputeShaderRHIRef RHICreateComputeShader(const FRHICreateShaderDesc& description) override;
        FComputePipelineStateRHIRef RHICreateComputePipelineState(const FComputePipelineStateInitializer& initializer) override;
        FGraphicsPipelineStateRHIRef RHICreateGraphicsPipelineState(const FGraphicsPipelineStateInitializer& initializer) override;
        FUniformBufferRHIRef RHICreateUniformBuffer(const void* contents, const FRHIUniformBufferLayout* layout, EUniformBufferUsage usage, EUniformBufferValidation validation) override;
        void RHIUpdateUniformBuffer(FRHICommandListBase& commandList, FRHIUniformBuffer* buffer, const void* contents) override;
        FRHIBufferInitializer RHICreateBufferInitializer(FRHICommandListBase& commandList, const FRHIBufferCreateDesc& description) override;
        FShaderResourceViewRHIRef RHICreateShaderResourceView(FRHICommandListBase& commandList, FRHIViewableResource* resource, const FRHIViewDesc& description) override;
        FUnorderedAccessViewRHIRef RHICreateUnorderedAccessView(FRHICommandListBase& commandList, FRHIViewableResource* resource, const FRHIViewDesc& description) override;
        void RHICreateTransition(FRHITransition* transition, const FRHITransitionCreateInfo& info) override;
        void RHIReleaseTransition(FRHITransition* transition) override;
        FRHIFinalizeContextsResult RHIFinalizeContexts(FRHIFinalizeContextsArgs&& args) override;
        void RHISubmitCommandLists(FRHISubmitCommandListsArgs&& args) override;
        void RHIProcessDeleteQueue() override;
        void RHIBlockUntilGPUIdle() override;
        FGPUFenceRHIRef RHICreateGPUFence(const FName& name) override;
        FStagingBufferRHIRef RHICreateStagingBuffer() override;
        void* RHILockStagingBuffer(FRHIStagingBuffer* buffer, FRHIGPUFence* fence, uint32 offset, uint32 size) override;
        void RHIUnlockStagingBuffer(FRHIStagingBuffer* buffer) override;
        void* RHILockBuffer(FRHICommandListBase& commandList, FRHIBuffer* buffer, uint32 offset, uint32 size, EResourceLockMode mode) override;
        void RHIUnlockBuffer(FRHICommandListBase& commandList, FRHIBuffer* buffer) override;
        FBlendStateRHIRef RHICreateBlendState(const FBlendStateInitializerRHI& initializer) override;
        FDepthStencilStateRHIRef RHICreateDepthStencilState(const FDepthStencilStateInitializerRHI& initializer) override;
        FRasterizerStateRHIRef RHICreateRasterizerState(const FRasterizerStateInitializerRHI& initializer) override;
        FSamplerStateRHIRef RHICreateSamplerState(const FSamplerStateInitializerRHI& initializer) override;
        FRHITextureInitializer RHICreateTextureInitializer(FRHICommandListBase& commandList, const FRHITextureCreateDesc& description) override;

    protected:
        DynamicRHI(Shader::AgcTarget target, std::uint32_t addressHigh);
        void ConfigureTransitionStorage();

    private:
        Shader::AgcTarget target;
        std::uint32_t addressHigh;
        std::vector<std::unique_ptr<CommandList>> pendingCommandLists;
        bool submissionFailed = false;
    };
}

#endif
