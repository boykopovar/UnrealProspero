#ifndef UNREALPROSPERO_WINDOWING_WINDOW_H
#define UNREALPROSPERO_WINDOWING_WINDOW_H

#include "GenericPlatform/GenericWindow.h"
#include "GenericPlatform/GenericApplicationMessageHandler.h"
#include "UnrealProspero/Video/Output.h"
#include <memory>
#include <atomic>

namespace UnrealProspero::Windowing
{
    class APPLICATIONCORE_API Window final : public FGenericWindow, public TSharedFromThis<Window>
    {
    public:
        Window(std::shared_ptr<Video::Output> output, const TSharedRef<FGenericApplicationMessageHandler>& messageHandler);
        void SetMessageHandler(const TSharedRef<FGenericApplicationMessageHandler>& messageHandler);
        void Initialize(const TSharedRef<FGenericWindowDefinition>& definition, bool show);
        std::shared_ptr<Video::Output> GetOutput() const;
        std::shared_ptr<const std::atomic<bool>> CaptureVisibility() const;
        bool IsDestroyed() const;
        void ReshapeWindow(int32 x, int32 y, int32 width, int32 height) override;
        bool GetFullScreenInfo(int32& x, int32& y, int32& width, int32& height) const override;
        void MoveWindowTo(int32 x, int32 y) override;
        void BringToFront(bool force = false) override;
        void HACK_ForceToFront() override;
        void Destroy() override;
        void Minimize() override;
        void Maximize() override;
        void Restore() override;
        void Show() override;
        void Hide() override;
        void SetWindowMode(EWindowMode::Type mode) override;
        EWindowMode::Type GetWindowMode() const override;
        bool IsMaximized() const override;
        bool IsMinimized() const override;
        bool IsVisible() const override;
        bool GetRestoredDimensions(int32& x, int32& y, int32& width, int32& height) override;
        void SetWindowFocus() override;
        void SetOpacity(float opacity) override;
        void Enable(bool enabled) override;
        bool IsPointInWindow(int32 x, int32 y) const override;
        int32 GetWindowBorderSize() const override;
        int32 GetWindowTitleBarSize() const override;
        void* GetOSWindowHandle() const override;
        bool IsForegroundWindow() const override;
        bool SupportsDynamicCursorLock() const override;
        bool IsFullscreenSupported() const override;
        void SetText(const TCHAR* const text) override;

    private:
        void requireInitialized() const;
        void setRestoredPosition(int32 x, int32 y);
        std::shared_ptr<Video::Output> output;
        TWeakPtr<FGenericApplicationMessageHandler> messageHandler;
        EWindowMode::Type windowMode = UNREALPROSPERO_TARGET_PS5 ? EWindowMode::Fullscreen : EWindowMode::Windowed;
        int32 restoredX = 0;
        int32 restoredY = 0;
        int32 width = 0;
        int32 height = 0;
        std::shared_ptr<std::atomic<bool>> visible = std::make_shared<std::atomic<bool>>(false);
        bool destroyed = false;
    };
}

#endif
