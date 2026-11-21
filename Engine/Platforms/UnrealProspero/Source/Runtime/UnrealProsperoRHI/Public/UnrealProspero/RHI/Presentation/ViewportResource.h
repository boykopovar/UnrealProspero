#ifndef UNREALPROSPERO_RHI_PRESENTATION_VIEWPORT_RESOURCE_H
#define UNREALPROSPERO_RHI_PRESENTATION_VIEWPORT_RESOURCE_H

#include "UnrealProspero/RHI/Presentation/SwapChain.h"
#include "UnrealProspero/RHI/Texture/TextureResource.h"
#include "UnrealProspero/Shader/ShaderBinary.h"
#include <atomic>
#include <memory>

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API ViewportResource final : public FRHIViewport
    {
    public:
        static FViewportRHIRef Create(void* windowHandle, uint32 width, uint32 height, bool fullscreen, EPixelFormat format, Shader::AgcTarget target, uint32 addressHigh);
        FTextureRHIRef GetBackBuffer();
        void Resize(uint32 width, uint32 height, bool fullscreen, EPixelFormat format);
        void EndFrame(CommandBuffer& commands, const FRHIPresentArgs& arguments);
        bool RequiresOutputTransition(const FRHIPresentArgs& arguments) const;
        void PresentOutputTransition(const FRHIPresentArgs& arguments);
        void* GetNativeSwapChain() const override;
        void* GetNativeBackBufferTexture() const override;
        void* GetNativeWindow(void** extra = nullptr) const override;
        void SetCustomPresent(FRHICustomPresent* present) override;

    private:
        ViewportResource(void* windowHandle, Shader::AgcTarget target, EPixelFormat format, std::shared_ptr<const std::atomic<bool>> visible, std::unique_ptr<SwapChain> chain, std::unique_ptr<FCriticalSection> mutex);
        static void validateMode(bool fullscreen, Shader::AgcTarget target);
        FTextureRHIRef acquireBackBuffer();
        void* windowHandle;
        std::shared_ptr<const std::atomic<bool>> visible;
        Shader::AgcTarget target;
        EPixelFormat format;
        std::unique_ptr<SwapChain> chain;
        std::array<TRefCountPtr<TextureResource>, 3> backBuffers;
        std::unique_ptr<FCriticalSection> mutex;
        bool highRefreshPending;
    };
}

#endif
