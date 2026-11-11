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
        DynamicRHI(Shader::AgcTarget target, std::uint32_t addressHigh);
        bool RHIGetTextureMemoryVisualizeData(FColor* textureData, int32 width, int32 height, int32 pitch, int32 pixelSize) override;
        void* RHIGetNativeDevice() override;
        uint64 RHIGetMinimumAlignmentForBufferBackedSRV(EPixelFormat format) override;
        FTextureRHIRef RHIAsyncCreateTexture2D(uint32 width, uint32 height, uint8 format, uint32 mipCount, ETextureCreateFlags flags, ERHIAccess initialState, void** initialMipData, uint32 initialMipCount, const TCHAR* debugName, FGraphEventRef& completionEvent) override;
        bool RHIGetAvailableResolutions(FScreenResolutionArray& resolutions, bool ignoreRefreshRate) override;
        void RHIGetSupportedResolution(uint32& width, uint32& height) override;
        void RHIGetTextureMemoryStats(FTextureMemoryStats& statistics) override;
        void Init() override;
        void Shutdown() override;
        const TCHAR* GetName() override;
        void RHITick(float deltaTime) override;
        void RHIReplaceResources(FRHICommandListBase& commandList, TArray<FRHIResourceReplaceInfo>&& replacements) override;
        void RHIUpdateTexture3D(FRHICommandListBase& commandList, FRHITexture* texture, uint32 mipIndex, const FUpdateTextureRegion3D& region, uint32 sourceRowPitch, uint32 sourceSlicePitch, const uint8* sourceData) override;
        void RHIUpdateTexture2D(FRHICommandListBase& commandList, FRHITexture* texture, uint32 mipIndex, const FUpdateTextureRegion2D& region, uint32 sourcePitch, const uint8* sourceData) override;
        FRHILockTextureResult RHILockTexture(FRHICommandListImmediate& commandList, const FRHILockTextureArgs& arguments) override;
        void RHIUnlockTexture(FRHICommandListImmediate& commandList, const FRHILockTextureArgs& arguments) override;
        FRHICalcTextureSizeResult RHICalcTexturePlatformSize(const FRHITextureDesc& description, uint32 firstMipIndex) override;
        void RHIRead3DSurfaceFloatData(FRHITexture* texture, FIntRect rectangle, FIntPoint zRange, TArray<FFloat16Color>& output) override;
        void RHIReadSurfaceFloatData(FRHITexture* texture, FIntRect rectangle, TArray<FFloat16Color>& output, ECubeFace cubeFace, int32 arrayIndex, int32 mipIndex) override;
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
        FBoundShaderStateRHIRef RHICreateBoundShaderState(FRHIVertexDeclaration* declaration, FRHIVertexShader* vertex, FRHIPixelShader* pixel, FRHIGeometryShader* geometry) override;
        FVertexShaderRHIRef RHICreateVertexShader(const FRHICreateShaderDesc& description) override;
        FGeometryShaderRHIRef RHICreateGeometryShader(const FRHICreateShaderDesc& description) override;
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
        IRHIComputeContext* RHIGetCommandContext(ERHIPipeline pipeline) override;
        IRHICommandContext* RHIGetDefaultContext() override;
        FRHIFinalizeContextsResult RHIFinalizeContexts(FRHIFinalizeContextsArgs&& args) override;
        void RHISubmitCommandLists(FRHISubmitCommandListsArgs&& args) override;
        void RHIProcessDeleteQueue() override;
        void RHIEndFrame(const FRHIEndFrameArgs& arguments) override;
        void RHIBlockUntilGPUIdle() override;
        FRenderQueryRHIRef RHICreateRenderQuery(ERenderQueryType type) override;
        bool RHIGetRenderQueryResult(FRHIRenderQuery* query, uint64& result, bool wait, uint32 gpuIndex = INDEX_NONE) override;
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
        void ConfigureTransitionStorage();

    private:
        Shader::AgcTarget target;
        std::uint32_t addressHigh;
        std::vector<std::unique_ptr<CommandList>> pendingCommandLists;
        std::unique_ptr<ComputeContext> defaultContext;
        bool submissionFailed = false;
        bool initialized = false;
        bool shutDown = false;
    };
}

#endif
