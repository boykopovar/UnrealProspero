#include "AnyUnreal/RHI/Device/DynamicRHI.h"
#include "AnyUnreal/RHI/Device/DeviceLimits.h"
#include "AnyUnreal/RHI/Device/ShaderPlatform.h"
#include "AnyUnreal/RHI/Texture/PixelFormat.h"
#include "AnyUnreal/RHI/Shader/GeometryShader.h"
#include "AnyUnreal/Video/Output.h"
#include "AnyUnreal/RHI/Presentation/ViewportResource.h"
#include "AnyUnreal/RHI/Context/GraphicsContext.h"
#include "AnyUnreal/RHI/Buffer/BufferInitializer.h"
#include "AnyUnreal/RHI/Buffer/BufferView.h"
#include "AnyUnreal/RHI/Buffer/TypedFormat.h"
#include "AnyUnreal/RHI/Buffer/StagingBuffer.h"
#include "AnyUnreal/RHI/Buffer/UniformBuffer.h"
#include "AnyUnreal/RHI/Pipeline/ComputePipeline.h"
#include "AnyUnreal/RHI/Pipeline/GraphicsPipeline.h"
#include "AnyUnreal/RHI/Transition/ResourceTransition.h"
#include "AnyUnreal/RHI/Command/CommandList.h"
#include "AnyUnreal/RHI/Command/GpuFence.h"
#include "AnyUnreal/RHI/Query/RenderQuery.h"
#include "AnyUnreal/RHI/State/SamplerState.h"
#include "AnyUnreal/RHI/State/RasterizerState.h"
#include "AnyUnreal/RHI/State/DepthStencilState.h"
#include "AnyUnreal/RHI/State/BlendState.h"
#include "AnyUnreal/RHI/State/VertexDeclaration.h"
#include "AnyUnreal/RHI/State/BoundShaderState.h"
#include "AnyUnreal/RHI/Texture/TextureInitializer.h"
#include "AnyUnreal/RHI/Texture/TextureView.h"
#include "AnyUnreal/RHI/Texture/SurfaceData.h"
#include "RHICommandList.h"
#include "RHIResourceReplace.h"
#include "RHIStats.h"
#include <stdexcept>
#include <set>
#include <limits>
#include <cmath>
#include <algorithm>
#include <bit>
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libSceAgc/libSceAgc.hpp"

namespace AnyUnreal::RHI
{
    FTextureRHIRef DynamicRHI::RHIAsyncCreateTexture2D(uint32 width, uint32 height, uint8 format, uint32 mipCount, ETextureCreateFlags flags, ERHIAccess initialState, void** initialMipData, uint32 initialMipCount, const TCHAR* debugName, FGraphEventRef& completionEvent)
    {
        if (!initialized || shutDown) throw std::runtime_error("Texture creation requires an initialized device");
        if (width == 0 || height == 0 || width > DeviceLimits::Texture2D || height > DeviceLimits::Texture2D) throw std::invalid_argument("Texture extent exceeds hardware limits");
        if (format == PF_Unknown || format >= PF_MAX) throw std::invalid_argument("Invalid texture format");
        const auto maximumMipCount = static_cast<uint32>(std::bit_width(std::max(width, height)));
        if (mipCount == 0) mipCount = maximumMipCount;
        if (mipCount > maximumMipCount) throw std::invalid_argument("Texture mip count exceeds its dimensions");
        if (initialMipCount > mipCount || (initialMipCount && !initialMipData)) throw std::invalid_argument("Invalid initial texture mip data");
        constexpr auto targetFlags = ETextureCreateFlags::RenderTargetable | ETextureCreateFlags::DepthStencilTargetable | ETextureCreateFlags::ResolveTargetable | ETextureCreateFlags::DepthStencilResolveTarget | ETextureCreateFlags::Presentable;
        if (EnumHasAnyFlags(flags, targetFlags)) throw std::invalid_argument("Asynchronous texture creation does not accept render targets");
        if (IsInvalidAccess(initialState)) throw std::invalid_argument("Invalid initial texture access");
        const auto description = FRHITextureCreateDesc::Create2D(debugName, static_cast<int32>(width), static_cast<int32>(height), static_cast<EPixelFormat>(format)).SetFlags(flags).SetInitialState(initialState).SetNumMips(static_cast<uint8>(mipCount));
        FTextureRHIRef result = initialMipCount ? new TextureResource(description, addressHigh, std::span<void* const>{initialMipData, initialMipCount}) : new TextureResource(description, addressHigh);
        completionEvent.SafeRelease();
        return result;
    }

    bool DynamicRHI::RHIGetAvailableResolutions(FScreenResolutionArray& resolutions, bool)
    {
        const auto status = Video::Output::AcquirePrimary()->GetResolution();
        uint32 refreshRate;
        switch (status.refreshRate)
        {
        case 3: refreshRate = 60; break;
        case 13: refreshRate = 120; break;
        default: throw std::runtime_error("Video output refresh rate is not implemented");
        }
        FScreenResolutionArray result;
        result.Add({status.fullWidth, status.fullHeight, refreshRate});
        resolutions = MoveTemp(result);
        return true;
    }

    void DynamicRHI::RHIGetSupportedResolution(uint32& width, uint32& height)
    {
        const auto status = Video::Output::AcquirePrimary()->GetResolution();
        width = status.fullWidth;
        height = status.fullHeight;
    }

    void DynamicRHI::RHIGetTextureMemoryStats(FTextureMemoryStats& statistics)
    {
        if (!initialized || shutDown) throw std::runtime_error("Texture memory statistics require an initialized device");
        FTextureMemoryStats result;
        result.NonStreamingMemorySize = BufferAllocation::GetTextureAllocationBytes();
        statistics = result;
    }

    bool DynamicRHI::RHIGetTextureMemoryVisualizeData(FColor*, int32, int32, int32, int32)
    {
        throw std::runtime_error("Texture allocation visualization is not implemented");
    }

    void* DynamicRHI::RHIGetNativeDevice()
    {
        throw std::runtime_error("AGC does not expose a native device handle");
    }

    uint64 DynamicRHI::RHIGetMinimumAlignmentForBufferBackedSRV(EPixelFormat format)
    {
        return GetBufferElementSize(ResolveTypedBufferFormat(format));
    }

    void DynamicRHI::Init()
    {
        if (initialized || shutDown) throw std::runtime_error("RHI initialization requires a new device");
        const auto platform = ResolveShaderPlatform(target);
        Kernel::CheckResult(sceAgcInit(11), "sceAgcInit");
        ConfigureShaderPlatform(platform);
        ConfigurePixelFormats();
        ConfigureDeviceLimits();
        ConfigureTransitionStorage();
        initialized = true;
    }

    void DynamicRHI::Shutdown()
    {
        if (!initialized || shutDown) throw std::runtime_error("RHI shutdown requires an initialized device");
        if (submissionFailed) throw std::runtime_error("GPU submission queue has failed");
        if (defaultContext)
        {
            auto commands = std::make_unique<CommandList>(std::move(defaultContext));
            pendingCommandLists.push_back(std::move(commands));
            try
            {
                pendingCommandLists.back()->Submit();
            }
            catch (...)
            {
                submissionFailed = true;
                throw;
            }
        }
        RHIBlockUntilGPUIdle();
        initialized = false;
        shutDown = true;
    }

    const TCHAR* DynamicRHI::GetName() { return TEXT("AnyUnrealAGC"); }

    void DynamicRHI::RHITick(float deltaTime)
    {
        if (!initialized || shutDown) throw std::runtime_error("RHI tick requires an initialized device");
        if (!std::isfinite(deltaTime) || deltaTime < 0) throw std::invalid_argument("Invalid RHI tick interval");
        RHIProcessDeleteQueue();
    }

    void DynamicRHI::RHIReplaceResources(FRHICommandListBase& commandList, TArray<FRHIResourceReplaceInfo>&& replacements)
    {
        std::vector<std::pair<TRefCountPtr<Buffer>, TRefCountPtr<Buffer>>> buffers;
        buffers.reserve(replacements.Num());
        for (const auto& replacement : replacements)
        {
            if (replacement.GetType() != FRHIResourceReplaceInfo::EType::Buffer) throw std::runtime_error("Ray tracing resource replacement is not implemented");
            const auto& pair = replacement.GetBuffer();
            if (!pair.Dst || pair.Dst == pair.Src) throw std::invalid_argument("Buffer replacement requires distinct source and destination resources");
            buffers.emplace_back(static_cast<Buffer*>(pair.Dst), static_cast<Buffer*>(pair.Src));
        }
        commandList.EnqueueLambdaMultiPipe(ERHIPipeline::Graphics, FRHICommandListBase::EThreadFence::Enabled, TEXT("AnyUnrealReplaceResources"), [buffers = std::move(buffers)](const FRHIContextArray&)
        {
            for (const auto& pair : buffers) pair.first->ReplaceStorage(pair.second.GetReference());
        });
    }

    FRenderQueryRHIRef DynamicRHI::RHICreateRenderQuery(ERenderQueryType type)
    {
        return new RenderQuery(type);
    }

    bool DynamicRHI::RHIGetRenderQueryResult(FRHIRenderQuery* query, uint64& result, bool wait, uint32 gpuIndex)
    {
        if (!query) throw std::invalid_argument("Render query is null");
        if (gpuIndex != 0 && gpuIndex != static_cast<uint32>(INDEX_NONE)) throw std::invalid_argument("Render query supports only GPU zero");
        if (wait) FRHICommandListExecutor::GetImmediateCommandList().ImmediateFlush(EImmediateFlushType::FlushRHIThread);
        return static_cast<RenderQuery*>(query)->TryGetResult(result, wait);
    }

    FBoundShaderStateRHIRef DynamicRHI::RHICreateBoundShaderState(FRHIVertexDeclaration* declaration, FRHIVertexShader* vertex, FRHIPixelShader* pixel, FRHIGeometryShader* geometry)
    {
        return new BoundShaderState(declaration, vertex, pixel, geometry);
    }

    DynamicRHI::DynamicRHI(Shader::AgcTarget target, std::uint32_t addressHigh) : target(target), addressHigh(addressHigh)
    {
        if (target != Shader::AgcTarget::Windows && target != Shader::AgcTarget::Linux && target != Shader::AgcTarget::Ps5) throw std::invalid_argument("Invalid AGC target");
        if (addressHigh > 0x7fff) throw std::invalid_argument("GPU address aperture exceeds user address space");
    }

    void DynamicRHI::RHIUpdateTexture3D(FRHICommandListBase& commandList, FRHITexture* texture, uint32 mipIndex, const FUpdateTextureRegion3D& region, uint32 sourceRowPitch, uint32 sourceSlicePitch, const uint8* sourceData)
    {
        if (mipIndex != 0) throw std::out_of_range("Texture mip does not exist");
        auto destination = ResolveTextureResource(texture).CaptureVolume();
        if (!sourceData || region.SrcX < 0 || region.SrcY < 0 || region.SrcZ < 0 || region.Width == 0 || region.Height == 0 || region.Depth == 0) throw std::invalid_argument("Invalid volume update source");
        if (region.DestX >= destination->GetWidth() || region.DestY >= destination->GetHeight() || region.DestZ >= destination->GetDepth() || region.Width > destination->GetWidth() - region.DestX || region.Height > destination->GetHeight() - region.DestY || region.Depth > destination->GetDepth() - region.DestZ) throw std::out_of_range("Volume update exceeds its destination");
        const auto pixelBytes = LinearTexture2D::CalculateRowBytes(1, destination->GetFormat());
        const auto rowBytes = region.Width * pixelBytes;
        const auto column = static_cast<std::size_t>(region.SrcX) * pixelBytes;
        if (column > sourceRowPitch || rowBytes > sourceRowPitch - column) throw std::invalid_argument("Volume update source row pitch is too small");
        const auto rows = static_cast<std::size_t>(region.SrcY) + region.Height;
        if (rows > sourceSlicePitch / sourceRowPitch) throw std::invalid_argument("Volume update source slice pitch is too small");
        const auto offset = static_cast<std::size_t>(region.SrcZ) * sourceSlicePitch + static_cast<std::size_t>(region.SrcY) * sourceRowPitch + column;
        const auto bytes = static_cast<std::size_t>(region.Depth - 1) * sourceSlicePitch + static_cast<std::size_t>(region.Height - 1) * sourceRowPitch + rowBytes;
        const auto limit = static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max());
        if (bytes > limit || offset > limit - bytes) throw std::overflow_error("Volume update source exceeds pointer range");
        auto upload = std::make_shared<VolumeUpload>(region.Width, region.Height, region.Depth, destination->GetFormat(), std::span{reinterpret_cast<const std::byte*>(sourceData) + offset, bytes}, sourceRowPitch, sourceSlicePitch, addressHigh);
        commandList.EnqueueLambda([destination = std::move(destination), upload = std::move(upload), x = region.DestX, y = region.DestY, z = region.DestZ](FRHICommandListBase& executingList)
        {
            static_cast<GraphicsContext&>(executingList.GetContext()).UploadVolume(destination, upload, x, y, z);
        });
    }

    void DynamicRHI::RHIUpdateTexture2D(FRHICommandListBase& commandList, FRHITexture* texture, uint32 mipIndex, const FUpdateTextureRegion2D& region, uint32 sourcePitch, const uint8* sourceData)
    {
        auto destination = ResolveTextureResource(texture).Capture();
        const auto& mip = destination->GetMipLayout(mipIndex);
        if (!sourceData || region.SrcX < 0 || region.SrcY < 0 || region.Width == 0 || region.Height == 0) throw std::invalid_argument("Invalid texture update source");
        if (region.DestX >= mip.Width || region.DestY >= mip.Height || region.Width > mip.Width - region.DestX || region.Height > mip.Height - region.DestY) throw std::out_of_range("Texture update exceeds its destination");
        const auto pixelBytes = mip.RowBytes / mip.Width;
        const auto rowBytes = region.Width * pixelBytes;
        const auto columnOffset = static_cast<std::size_t>(region.SrcX) * pixelBytes;
        if (columnOffset > sourcePitch || rowBytes > sourcePitch - columnOffset) throw std::invalid_argument("Texture update source pitch is too small");
        const auto sourceOffset = static_cast<std::size_t>(region.SrcY) * sourcePitch + columnOffset;
        const auto sourceBytes = (region.Height - 1) * static_cast<std::size_t>(sourcePitch) + rowBytes;
        if (sourceOffset > static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max()) - sourceBytes) throw std::overflow_error("Texture update source exceeds pointer range");
        auto upload = std::make_shared<TextureUpload>(region.Width, region.Height, destination->GetFormat(), std::span{reinterpret_cast<const std::byte*>(sourceData) + sourceOffset, sourceBytes}, sourcePitch, addressHigh);
        commandList.EnqueueLambda([destination = std::move(destination), upload = std::move(upload), x = region.DestX, y = region.DestY, mipIndex](FRHICommandListBase& executingList)
        {
            static_cast<GraphicsContext&>(executingList.GetContext()).UploadTexture(destination, upload, x, y, mipIndex);
        });
    }

    FRHILockTextureResult DynamicRHI::RHILockTexture(FRHICommandListImmediate& commandList, const FRHILockTextureArgs& arguments)
    {
        if (arguments.FaceIndex != 0 || arguments.ArrayIndex != 0) throw std::out_of_range("Texture lock subresource does not exist");
        if (arguments.LockMode != RLM_ReadOnly && arguments.LockMode != RLM_WriteOnly) throw std::invalid_argument("Invalid texture lock mode");
        auto& texture = ResolveTextureResource(arguments.Texture);
        if (commandList.IsExecuting()) throw std::runtime_error("Texture lock requires a non-executing immediate command list");
        if (arguments.LockMode == RLM_ReadOnly) commandList.SubmitAndBlockUntilGPUIdle();
        else if (arguments.bNeedsDefaultRHIFlush) commandList.ImmediateFlush(EImmediateFlushType::FlushRHIThread);
        std::size_t size = 0;
        std::size_t pitch = 0;
        auto* data = texture.Lock(arguments.LockMode, arguments.MipIndex, size, pitch);
        return {data, size, static_cast<uint32>(pitch)};
    }

    void DynamicRHI::RHIUnlockTexture(FRHICommandListImmediate& commandList, const FRHILockTextureArgs& arguments)
    {
        if (arguments.FaceIndex != 0 || arguments.ArrayIndex != 0) throw std::out_of_range("Texture unlock subresource does not exist");
        if (commandList.IsExecuting()) throw std::runtime_error("Texture unlock requires a non-executing immediate command list");
        auto& texture = ResolveTextureResource(arguments.Texture);
        auto locked = texture.Unlock(arguments.LockMode, arguments.MipIndex);
        if (locked.Mode == RLM_ReadOnly) return;
        const auto storage = texture.Capture();
        const auto& mip = storage->GetMipLayout(locked.MipLevel);
        const FUpdateTextureRegion2D region{0, 0, 0, 0, mip.Width, mip.Height};
        RHIUpdateTexture2D(commandList, &texture, locked.MipLevel, region, static_cast<uint32>(locked.Pitch), reinterpret_cast<const uint8*>(locked.Data.data()));
        if (arguments.bNeedsDefaultRHIFlush) commandList.ImmediateFlush(EImmediateFlushType::FlushRHIThread);
    }

    FRHICalcTextureSizeResult DynamicRHI::RHICalcTexturePlatformSize(const FRHITextureDesc& description, uint32 firstMipIndex)
    {
        if (firstMipIndex != 0) throw std::out_of_range("Texture mip does not exist");
        return {TextureResource::CalculateAllocationSize(description), static_cast<uint32>(TextureResource::CalculateAlignment(description))};
    }

    void DynamicRHI::RHIRead3DSurfaceFloatData(FRHITexture* texture, FIntRect rectangle, FIntPoint zRange, TArray<FFloat16Color>& output)
    {
        auto storage = ResolveTextureResource(texture).CaptureVolume();
        auto& commandList = FRHICommandListExecutor::GetImmediateCommandList();
        if (commandList.IsExecuting()) throw std::runtime_error("Volume read cannot execute inside a command list");
        commandList.SubmitAndBlockUntilGPUIdle();
        ReadVolumeHalfColors(*storage, rectangle, zRange, output);
    }

    void DynamicRHI::RHIReadSurfaceFloatData(FRHITexture* texture, FIntRect rectangle, TArray<FFloat16Color>& output, ECubeFace cubeFace, int32 arrayIndex, int32 mipIndex)
    {
        if (arrayIndex != 0 || mipIndex < 0 || cubeFace < CubeFace_PosX || cubeFace > CubeFace_MAX) throw std::invalid_argument("Surface read selects an unavailable subresource");
        auto storage = ResolveTextureResource(texture).Capture();
        auto& commandList = FRHICommandListExecutor::GetImmediateCommandList();
        if (commandList.IsExecuting()) throw std::runtime_error("Surface read cannot execute inside a command list");
        commandList.SubmitAndBlockUntilGPUIdle();
        ReadSurfaceHalfColors(*storage, rectangle, output, static_cast<std::uint32_t>(mipIndex));
    }

    void DynamicRHI::RHIReadSurfaceData(FRHITexture* texture, FIntRect rectangle, TArray<FColor>& output, FReadSurfaceDataFlags flags)
    {
        auto storage = ResolveTextureResource(texture).Capture();
        auto& commandList = FRHICommandListExecutor::GetImmediateCommandList();
        if (commandList.IsExecuting()) throw std::runtime_error("Surface read cannot execute inside a command list");
        commandList.SubmitAndBlockUntilGPUIdle();
        ReadSurfaceColors(*storage, rectangle, flags, output);
    }

    void DynamicRHI::RHIReadSurfaceData(FRHITexture* texture, FIntRect rectangle, TArray<FLinearColor>& output, FReadSurfaceDataFlags flags)
    {
        auto storage = ResolveTextureResource(texture).Capture();
        auto& commandList = FRHICommandListExecutor::GetImmediateCommandList();
        if (commandList.IsExecuting()) throw std::runtime_error("Surface read cannot execute inside a command list");
        commandList.SubmitAndBlockUntilGPUIdle();
        ReadSurfaceColors(*storage, rectangle, flags, output);
    }

    void DynamicRHI::RHIMapStagingSurface(FRHITexture* texture, FRHIGPUFence* fence, void*& data, int32& width, int32& height, uint32 gpuIndex)
    {
        if (gpuIndex != 0 && gpuIndex != static_cast<uint32>(INDEX_NONE)) throw std::invalid_argument("Staging texture supports only GPU zero");
        auto& resource = ResolveTextureResource(texture);
        if (!EnumHasAnyFlags(resource.GetDesc().Flags, ETextureCreateFlags::CPUReadback)) throw std::invalid_argument("Texture was not created for CPU readback");
        auto& commandList = FRHICommandListExecutor::GetImmediateCommandList();
        if (commandList.IsExecuting()) throw std::runtime_error("Staging texture mapping cannot execute inside a command list");
        if (fence)
        {
            if (!fence->Poll(FRHIGPUMask::GPU0())) fence->Wait(commandList, FRHIGPUMask::GPU0());
        }
        else commandList.SubmitAndBlockUntilGPUIdle();
        const auto storage = resource.Capture();
        data = resource.MapReadback();
        width = static_cast<int32>(storage->GetWidth());
        height = static_cast<int32>(storage->GetHeight());
    }

    void DynamicRHI::RHIUnmapStagingSurface(FRHITexture* texture, uint32 gpuIndex)
    {
        if (gpuIndex != 0 && gpuIndex != static_cast<uint32>(INDEX_NONE)) throw std::invalid_argument("Staging texture supports only GPU zero");
        ResolveTextureResource(texture).UnmapReadback();
    }

    FViewportRHIRef DynamicRHI::RHICreateViewport(void* windowHandle, uint32 width, uint32 height, bool fullscreen, EPixelFormat format)
    {
        return new ViewportResource(windowHandle, width, height, fullscreen, format, target, addressHigh);
    }

    void DynamicRHI::RHIResizeViewport(FRHIViewport* viewport, uint32 width, uint32 height, bool fullscreen, EPixelFormat format)
    {
        if (!viewport) throw std::invalid_argument("Viewport is null");
        auto& commandList = FRHICommandListExecutor::GetImmediateCommandList();
        if (commandList.IsExecuting()) throw std::runtime_error("Viewport resize cannot execute inside a command list");
        commandList.SubmitAndBlockUntilGPUIdle();
        static_cast<ViewportResource*>(viewport)->Resize(width, height, fullscreen, format);
    }

    FTextureRHIRef DynamicRHI::RHIGetViewportBackBuffer(FRHIViewport* viewport)
    {
        if (!viewport) throw std::invalid_argument("Viewport is null");
        return static_cast<ViewportResource*>(viewport)->GetBackBuffer();
    }

    void DynamicRHI::RHIEndDrawingViewport(FRHICommandListImmediate& commandList, FRHIViewport* viewport, const FRHIPresentArgs& arguments)
    {
        if (!viewport) throw std::invalid_argument("Viewport is null");
        if (commandList.IsExecuting()) throw std::runtime_error("Viewport present cannot execute inside a command list");
        commandList.SubmitAndBlockUntilGPUIdle();
        static_cast<ViewportResource*>(viewport)->EndFrame(arguments);
    }

    uint32 DynamicRHI::RHIComputeMemorySize(FRHITexture* texture)
    {
        const auto size = ResolveTextureResource(texture).GetAllocationSize();
        if (size > std::numeric_limits<uint32>::max()) throw std::overflow_error("Texture allocation exceeds the RHI memory-size range");
        return static_cast<uint32>(size);
    }

    FGraphicsPipelineStateRHIRef DynamicRHI::RHICreateGraphicsPipelineState(const FGraphicsPipelineStateInitializer& initializer)
    {
        return new GraphicsPipeline(initializer);
    }

    FVertexDeclarationRHIRef DynamicRHI::RHICreateVertexDeclaration(const FVertexDeclarationElementList& elements)
    {
        return new VertexDeclaration(elements);
    }

    FRHITextureInitializer DynamicRHI::RHICreateTextureInitializer(FRHICommandListBase& commandList, const FRHITextureCreateDesc& description)
    {
        return CreateTextureInitializer(commandList, description, addressHigh);
    }

    FBlendStateRHIRef DynamicRHI::RHICreateBlendState(const FBlendStateInitializerRHI& initializer)
    {
        return new BlendState(initializer);
    }

    FDepthStencilStateRHIRef DynamicRHI::RHICreateDepthStencilState(const FDepthStencilStateInitializerRHI& initializer)
    {
        return new DepthStencilState(initializer);
    }

    FRasterizerStateRHIRef DynamicRHI::RHICreateRasterizerState(const FRasterizerStateInitializerRHI& initializer)
    {
        return new RasterizerState(initializer);
    }

    FSamplerStateRHIRef DynamicRHI::RHICreateSamplerState(const FSamplerStateInitializerRHI& initializer)
    {
        return new SamplerState(initializer);
    }

    void* DynamicRHI::RHILockBuffer(FRHICommandListBase& commandList, FRHIBuffer* buffer, uint32 offset, uint32 size, EResourceLockMode mode)
    {
        if (!buffer) throw std::invalid_argument("Buffer lock requires a buffer");
        if (mode == RLM_ReadOnly)
        {
            if (!commandList.IsImmediate() || commandList.IsExecuting()) throw std::runtime_error("Synchronous buffer read requires a non-executing immediate command list");
            commandList.GetAsImmediate().SubmitAndBlockUntilGPUIdle();
        }
        return static_cast<Buffer*>(buffer)->Lock(offset, size, mode);
    }

    void DynamicRHI::RHIUnlockBuffer(FRHICommandListBase& commandList, FRHIBuffer* buffer)
    {
        if (!buffer) throw std::invalid_argument("Buffer unlock requires a buffer");
        auto range = static_cast<Buffer*>(buffer)->Unlock();
        if (range.Mode == RLM_ReadOnly) return;
        commandList.EnqueueLambda([buffer = TRefCountPtr<Buffer>(static_cast<Buffer*>(buffer)), range = std::move(range)](FRHICommandListBase& executingList)
        {
            static_cast<ComputeContext&>(executingList.GetContext()).UploadBuffer(buffer.GetReference(), range.Offset, range.Data);
        });
        if (commandList.IsTopOfPipe() || commandList.Bypass()) commandList.RHIThreadFence(true);
    }

    FStagingBufferRHIRef DynamicRHI::RHICreateStagingBuffer()
    {
        return new StagingBuffer();
    }

    void* DynamicRHI::RHILockStagingBuffer(FRHIStagingBuffer* buffer, FRHIGPUFence* fence, uint32 offset, uint32 size)
    {
        if (!buffer) throw std::invalid_argument("Staging buffer is null");
        if (fence && !fence->Poll()) throw std::runtime_error("Staging buffer fence has not completed");
        return buffer->Lock(offset, size);
    }

    void DynamicRHI::RHIUnlockStagingBuffer(FRHIStagingBuffer* buffer)
    {
        if (!buffer) throw std::invalid_argument("Staging buffer is null");
        buffer->Unlock();
    }

    FGPUFenceRHIRef DynamicRHI::RHICreateGPUFence(const FName& name)
    {
        return new GpuFence(name);
    }

    FVertexShaderRHIRef DynamicRHI::RHICreateVertexShader(const FRHICreateShaderDesc& description)
    {
        return new VertexShader(description.Code, FShaderHash{}, target);
    }

    FGeometryShaderRHIRef DynamicRHI::RHICreateGeometryShader(const FRHICreateShaderDesc& description)
    {
        return new GeometryShader(description.Code, FShaderHash{}, target);
    }

    FPixelShaderRHIRef DynamicRHI::RHICreatePixelShader(const FRHICreateShaderDesc& description)
    {
        return new PixelShader(description.Code, FShaderHash{}, target);
    }

    FComputeShaderRHIRef DynamicRHI::RHICreateComputeShader(const FRHICreateShaderDesc& description)
    {
        return new ComputeShader(description.Code, FShaderHash{}, target);
    }

    FComputePipelineStateRHIRef DynamicRHI::RHICreateComputePipelineState(const FComputePipelineStateInitializer& initializer)
    {
        if (!initializer.ComputeShader) throw std::invalid_argument("Compute pipeline requires a shader");
        if (!initializer.SpecializationConstants.IsEmpty() || initializer.ComputeShader->HasSpecializationConstants()) throw std::runtime_error("Shader specialization constants are not implemented");
        return new ComputePipeline(*static_cast<ComputeShader*>(initializer.ComputeShader));
    }

    FUniformBufferRHIRef DynamicRHI::RHICreateUniformBuffer(const void* contents, const FRHIUniformBufferLayout* layout, EUniformBufferUsage usage, EUniformBufferValidation validation)
    {
        return new UniformBuffer(layout, contents, usage, validation, addressHigh);
    }

    FRHIBufferInitializer DynamicRHI::RHICreateBufferInitializer(FRHICommandListBase& commandList, const FRHIBufferCreateDesc& description)
    {
        return CreateBufferInitializer(commandList, description, addressHigh);
    }

    void DynamicRHI::RHIUpdateUniformBuffer(FRHICommandListBase& commandList, FRHIUniformBuffer* buffer, const void* contents)
    {
        if (!buffer) throw std::invalid_argument("Uniform buffer is null");
        static_cast<UniformBuffer*>(buffer)->Update(commandList, contents);
    }

    FShaderResourceViewRHIRef DynamicRHI::RHICreateShaderResourceView(FRHICommandListBase&, FRHIViewableResource* resource, const FRHIViewDesc& description)
    {
        if (!resource) throw std::invalid_argument("Shader resource view resource is null");
        if (resource->GetType() == RRT_Buffer) return new BufferShaderResourceView(*static_cast<Buffer*>(resource), description);
        if (resource->GetType() == RRT_Texture) return new TextureShaderResourceView(*static_cast<TextureResource*>(resource), description);
        throw std::invalid_argument("Shader resource view requires a buffer or texture");
    }

    FUnorderedAccessViewRHIRef DynamicRHI::RHICreateUnorderedAccessView(FRHICommandListBase&, FRHIViewableResource* resource, const FRHIViewDesc& description)
    {
        if (!resource) throw std::invalid_argument("Unordered access view resource is null");
        if (resource->GetType() == RRT_Buffer) return new BufferUnorderedAccessView(*static_cast<Buffer*>(resource), description);
        if (resource->GetType() == RRT_Texture) return new TextureUnorderedAccessView(*static_cast<TextureResource*>(resource), description);
        throw std::invalid_argument("Unordered access view requires a buffer or texture");
    }

    void DynamicRHI::ConfigureTransitionStorage()
    {
        GRHITransitionPrivateData_SizeInBytes = sizeof(TransitionData);
        GRHITransitionPrivateData_AlignInBytes = alignof(TransitionData);
    }

    void DynamicRHI::RHICreateTransition(FRHITransition* transition, const FRHITransitionCreateInfo& info)
    {
        if (!transition) throw std::invalid_argument("Transition storage is null");
        if (GRHITransitionPrivateData_SizeInBytes != sizeof(TransitionData) || GRHITransitionPrivateData_AlignInBytes != alignof(TransitionData)) throw std::runtime_error("Transition storage is not configured");
        InitializeResourceTransition(*transition, info, addressHigh);
    }

    void DynamicRHI::RHIReleaseTransition(FRHITransition* transition)
    {
        if (!transition) throw std::invalid_argument("Transition storage is null");
        DestroyResourceTransition(*transition);
    }

    void DynamicRHI::RHISubmitCommandLists(FRHISubmitCommandListsArgs&& args)
    {
        if (!initialized || shutDown) throw std::runtime_error("RHI command processing requires an initialized device");
        if (submissionFailed) throw std::runtime_error("GPU submission queue has failed");
        std::set<IRHIPlatformCommandList*> uniqueLists;
        for (const auto& list : pendingCommandLists) uniqueLists.insert(list.get());
        for (auto* list : args.CommandLists)
        {
            if (!list || !uniqueLists.insert(list).second) throw std::invalid_argument("Command list submission contains a null or duplicate list");
        }
        const auto first = pendingCommandLists.size();
        pendingCommandLists.reserve(first + args.CommandLists.Num());
        for (auto* list : args.CommandLists) pendingCommandLists.emplace_back(static_cast<CommandList*>(list));
        args.CommandLists.Reset();
        try
        {
            for (auto index = first; index < pendingCommandLists.size(); ++index) pendingCommandLists[index]->Submit();
        }
        catch (...)
        {
            submissionFailed = true;
            throw;
        }
    }

    void DynamicRHI::RHIEndFrame(const FRHIEndFrameArgs& arguments)
    {
        if (arguments.bProfileNextFrame) throw std::runtime_error("GPU frame timing capture is not implemented");
        RHIProcessDeleteQueue();
    }

    void DynamicRHI::RHIProcessDeleteQueue()
    {
        if (submissionFailed) throw std::runtime_error("GPU submission queue has failed");
        std::erase_if(pendingCommandLists, [](const auto& list) { return list->IsComplete(); });
    }

    void DynamicRHI::RHIBlockUntilGPUIdle()
    {
        if (submissionFailed) throw std::runtime_error("GPU submission queue has failed");
        for (const auto& list : pendingCommandLists) list->Wait(10000000);
        pendingCommandLists.clear();
    }

    IRHIComputeContext* DynamicRHI::RHIGetCommandContext(ERHIPipeline pipeline)
    {
        if (!initialized || shutDown) throw std::runtime_error("RHI command processing requires an initialized device");
        if (pipeline != ERHIPipeline::Graphics) throw std::runtime_error("Async compute command contexts are not implemented");
        return new GraphicsContext(65536, addressHigh);
    }

    IRHICommandContext* DynamicRHI::RHIGetDefaultContext()
    {
        if (!initialized || shutDown) throw std::runtime_error("RHI command processing requires an initialized device");
        if (submissionFailed) throw std::runtime_error("GPU submission queue has failed");
        if (!defaultContext) defaultContext = std::make_unique<GraphicsContext>(65536, addressHigh);
        return static_cast<GraphicsContext*>(defaultContext.get());
    }

    FDynamicRHI::FRHIFinalizeContextsResult DynamicRHI::RHIFinalizeContexts(FRHIFinalizeContextsArgs&& args)
    {
        if (!initialized || shutDown) throw std::runtime_error("RHI command processing requires an initialized device");
        if (args.UploadContext || args.Contexts[ERHIPipeline::AsyncCompute]) throw std::runtime_error("Upload and async compute context finalization is not implemented");
        FRHIFinalizeContextsResult result;
        if (auto* context = args.Contexts[ERHIPipeline::Graphics])
        {
            auto owned = context == defaultContext.get() ? std::move(defaultContext) : std::unique_ptr<ComputeContext>(static_cast<ComputeContext*>(context));
            result.CommandLists[ERHIPipeline::Graphics] = std::make_unique<CommandList>(std::move(owned)).release();
        }
        return result;
    }
}
