#include "AnyUnreal/RHI/Presentation/ViewportResource.h"
#include "Misc/ScopeLock.h"
#include "AnyUnreal/Windowing/Window.h"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    namespace
    {
        TextureFormat storageFormat(EPixelFormat format)
        {
            if (format == PF_B8G8R8A8) return TextureFormat::Bgra8Unorm;
            if (format == PF_R8G8B8A8) return TextureFormat::Rgba8Unorm;
            throw std::invalid_argument("Viewport requires a supported display pixel format");
        }
    }

    ViewportResource::ViewportResource(void* windowHandle, uint32 width, uint32 height, bool fullscreen, EPixelFormat format, Shader::AgcTarget target, uint32 addressHigh) : windowHandle(windowHandle), target(target), format(format == PF_Unknown ? PF_B8G8R8A8 : format)
    {
        validateMode(fullscreen);
        if (!windowHandle) throw std::invalid_argument("Viewport requires an initialized game window");
        auto& window = *static_cast<Windowing::Window*>(windowHandle);
        auto output = window.GetOutput();
        visible = window.CaptureVisibility();
        chain = std::make_unique<SwapChain>(std::move(output), width, height, storageFormat(this->format), addressHigh);
    }

    void ViewportResource::validateMode(bool fullscreen) const
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
            const auto description = FRHITextureCreateDesc::Create2D(TEXT("AnyUnrealBackBuffer"), static_cast<int32>(storage->GetWidth()), static_cast<int32>(storage->GetHeight()), format).SetFlags(ETextureCreateFlags::RenderTargetable | ETextureCreateFlags::ShaderResource | ETextureCreateFlags::Presentable | ETextureCreateFlags::NoTiling).SetInitialState(ERHIAccess::Present);
            buffer = new TextureResource(description, storage);
        }
        return buffer.GetReference();
    }

    FTextureRHIRef ViewportResource::GetBackBuffer()
    {
        FScopeLock lock(&mutex);
        return acquireBackBuffer();
    }

    void ViewportResource::Resize(uint32 width, uint32 height, bool fullscreen, EPixelFormat nextFormat)
    {
        FScopeLock lock(&mutex);
        validateMode(fullscreen);
        const auto chosenFormat = nextFormat == PF_Unknown ? format : nextFormat;
        const auto nativeFormat = storageFormat(chosenFormat);
        chain->Discard();
        chain->Resize(width, height, nativeFormat);
        for (auto& buffer : backBuffers) buffer.SafeRelease();
        format = chosenFormat;
    }

    void ViewportResource::EndFrame(const FRHIPresentArgs& arguments)
    {
        FScopeLock lock(&mutex);
        if (!arguments.bPresent || !visible->load(std::memory_order_acquire))
        {
            chain->Discard();
            return;
        }
        if (!arguments.bLockToVsync) throw std::runtime_error("Unsynchronized viewport presentation is not implemented");
        static_cast<void>(acquireBackBuffer());
        if (PresentBeginLatencyCallback) PresentBeginLatencyCallback();
        chain->Present(1);
        if (PresentEndLatencyCallback) PresentEndLatencyCallback();
    }

    void* ViewportResource::GetNativeSwapChain() const { return chain.get(); }

    void* ViewportResource::GetNativeBackBufferTexture() const
    {
        FScopeLock lock(&mutex);
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
