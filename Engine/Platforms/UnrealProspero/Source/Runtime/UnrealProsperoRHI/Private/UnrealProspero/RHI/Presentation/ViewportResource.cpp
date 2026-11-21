#include "UnrealProspero/RHI/Presentation/ViewportResource.h"
#include "Misc/ScopeLock.h"
#include "HAL/IConsoleManager.h"
#include "UnrealProspero/Windowing/Window.h"
#include <stdexcept>
#include <string>
#include <utility>

namespace UnrealProspero::RHI
{
    namespace
    {
        TAutoConsoleVariable<int32> highRefreshRate(TEXT("r.UnrealProspero.HighRefreshRate"), 0, TEXT("Request 120 Hz output on PS5."), ECVF_RenderThreadSafe);

        TextureFormat storageFormat(EPixelFormat format)
        {
            if (format == PF_B8G8R8A8) return TextureFormat::Bgra8Unorm;
            if (format == PF_R8G8B8A8) return TextureFormat::Rgba8Unorm;
            if (format == PF_A2B10G10R10) return TextureFormat::Rgb10A2Unorm;
            throw std::invalid_argument("Viewport display pixel format is not implemented: " + std::to_string(static_cast<int>(format)));
        }
    }

    FViewportRHIRef ViewportResource::Create(void* windowHandle, uint32 width, uint32 height, bool fullscreen, EPixelFormat format, Shader::AgcTarget target, uint32 addressHigh)
    {
        validateMode(fullscreen, target);
        if (!windowHandle) throw std::invalid_argument("Viewport requires an initialized game window");
        auto& window = *static_cast<Windowing::Window*>(windowHandle);
        auto output = window.GetOutput();
        auto visible = window.CaptureVisibility();
        const auto chosenFormat = format == PF_Unknown ? PF_B8G8R8A8 : format;
        auto mutex = std::make_unique<FCriticalSection>();
        auto chain = std::make_unique<SwapChain>(std::move(output), width, height, storageFormat(chosenFormat), addressHigh);
        return new ViewportResource(windowHandle, target, chosenFormat, std::move(visible), std::move(chain), std::move(mutex));
    }

    ViewportResource::ViewportResource(void* windowHandle, Shader::AgcTarget target, EPixelFormat format, std::shared_ptr<const std::atomic<bool>> visible, std::unique_ptr<SwapChain> chain, std::unique_ptr<FCriticalSection> mutex) : windowHandle(windowHandle), visible(std::move(visible)), target(target), format(format), chain(std::move(chain)), mutex(std::move(mutex)), highRefreshPending(target == Shader::AgcTarget::Ps5 && highRefreshRate.GetValueOnAnyThread() != 0) {}

    void ViewportResource::validateMode(bool fullscreen, Shader::AgcTarget target)
    {
        switch (target)
        {
        case Shader::AgcTarget::Ps5:
            if (!fullscreen) throw std::invalid_argument("PS5 viewport requires fullscreen presentation");
            break;
        case Shader::AgcTarget::Windows:
        case Shader::AgcTarget::Linux:
            if (fullscreen) throw std::runtime_error("Host fullscreen control is not implemented");
            break;
        default: throw std::invalid_argument("Invalid viewport platform");
        }
    }

    FTextureRHIRef ViewportResource::acquireBackBuffer()
    {
        const auto storage = chain->Acquire();
        auto& buffer = backBuffers[chain->GetBufferIndex()];
        if (!buffer)
        {
            const auto description = FRHITextureCreateDesc::Create2D(TEXT("UnrealProsperoBackBuffer"), static_cast<int32>(storage->GetWidth()), static_cast<int32>(storage->GetHeight()), format).SetFlags(ETextureCreateFlags::RenderTargetable | ETextureCreateFlags::ShaderResource | ETextureCreateFlags::Presentable).SetInitialState(ERHIAccess::Present);
            buffer = new TextureResource(description, storage);
        }
        return buffer.GetReference();
    }

    FTextureRHIRef ViewportResource::GetBackBuffer()
    {
        FScopeLock lock(mutex.get());
        return acquireBackBuffer();
    }

    void ViewportResource::Resize(uint32 width, uint32 height, bool fullscreen, EPixelFormat nextFormat)
    {
        FScopeLock lock(mutex.get());
        validateMode(fullscreen, target);
        const auto chosenFormat = nextFormat == PF_Unknown ? format : nextFormat;
        const auto nativeFormat = storageFormat(chosenFormat);
        chain->Discard();
        chain->Resize(width, height, nativeFormat);
        for (auto& buffer : backBuffers) buffer.SafeRelease();
        format = chosenFormat;
    }

    bool ViewportResource::RequiresOutputTransition(const FRHIPresentArgs& arguments) const
    {
        FScopeLock lock(mutex.get());
        return highRefreshPending && arguments.bPresent && visible->load(std::memory_order_acquire);
    }

    void ViewportResource::PresentOutputTransition(const FRHIPresentArgs& arguments)
    {
        FScopeLock lock(mutex.get());
        if (!highRefreshPending || !arguments.bPresent || !visible->load(std::memory_order_acquire)) throw std::logic_error("Viewport output transition is not pending");
        static_cast<void>(acquireBackBuffer());
        if (PresentBeginLatencyCallback) PresentBeginLatencyCallback();
        chain->Present(arguments.bLockToVsync ? 1 : 2);
        chain->ConfigureHighRefresh();
        highRefreshPending = false;
        if (PresentEndLatencyCallback) PresentEndLatencyCallback();
        UE_LOG(LogRHI, Display, TEXT("UnrealProspero video output configured for 120 Hz"));
    }

    void ViewportResource::EndFrame(CommandBuffer& commands, const FRHIPresentArgs& arguments)
    {
        FScopeLock lock(mutex.get());
        if (!arguments.bPresent || !visible->load(std::memory_order_acquire))
        {
            chain->Discard();
            return;
        }
        static_cast<void>(acquireBackBuffer());
        if (PresentBeginLatencyCallback) PresentBeginLatencyCallback();
        chain->Present(commands, arguments.bLockToVsync ? 1 : 2);
        if (PresentEndLatencyCallback) PresentEndLatencyCallback();
    }

    void* ViewportResource::GetNativeSwapChain() const { return chain.get(); }

    void* ViewportResource::GetNativeBackBufferTexture() const
    {
        FScopeLock lock(mutex.get());
        return chain->Acquire().get();
    }

    void* ViewportResource::GetNativeWindow(void** extra) const
    {
        if (extra) *extra = chain.get();
        return windowHandle;
    }

    void ViewportResource::SetCustomPresent(FRHICustomPresent* present)
    {
        if (present) throw std::runtime_error("Custom viewport presentation is not implemented");
    }
}
