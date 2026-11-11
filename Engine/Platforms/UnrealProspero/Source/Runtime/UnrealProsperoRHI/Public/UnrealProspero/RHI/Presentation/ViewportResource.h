#ifndef UNREALPROSPERO_RHI_PRESENTATION_VIEWPORT_RESOURCE_H
#define UNREALPROSPERO_RHI_PRESENTATION_VIEWPORT_RESOURCE_H

#include "UnrealProspero/RHI/Presentation/SwapChain.h"
#include "UnrealProspero/RHI/Texture/TextureResource.h"
#include "UnrealProspero/Shader/ShaderBinary.h"
#include <atomic>

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API ViewportResource final : public FRHIViewport
    {
    public:
        ViewportResource(void* windowHandle, uint32 width, uint32 height, bool fullscreen, EPixelFormat format, Shader::AgcTarget target, uint32 addressHigh);
        FTextureRHIRef GetBackBuffer();
        void Resize(uint32 width, uint32 height, bool fullscreen, EPixelFormat format);
        void EndFrame(const FRHIPresentArgs& arguments);
        void* GetNativeSwapChain() const override;
        void* GetNativeBackBufferTexture() const override;
        void* GetNativeWindow(void** extra = nullptr) const override;
        void SetCustomPresent(FRHICustomPresent* present) override;

    private:
        void validateMode(bool fullscreen) const;
        FTextureRHIRef acquireBackBuffer();
        void* windowHandle;
        std::shared_ptr<const std::atomic<bool>> visible;
        Shader::AgcTarget target;
        EPixelFormat format;
        std::unique_ptr<SwapChain> chain;
        std::array<TRefCountPtr<TextureResource>, 3> backBuffers;
        mutable FCriticalSection mutex;
    };
}

#endif
