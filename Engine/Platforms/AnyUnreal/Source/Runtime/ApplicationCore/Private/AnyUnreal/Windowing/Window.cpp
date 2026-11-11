#include "AnyUnreal/Windowing/Window.h"
#include "AnyUnreal/AnyUnrealPlatformApplicationMisc.h"
#include "GenericPlatform/GenericWindowDefinition.h"
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace AnyUnreal::Windowing
{
    Window::Window(std::shared_ptr<Video::Output> output, const TSharedRef<FGenericApplicationMessageHandler>& messageHandler) : output(std::move(output)), messageHandler(messageHandler)
    {
        if (!this->output) throw std::invalid_argument("Game window requires a video output");
        static_cast<void>(this->output->GetHandle());
    }

    void Window::SetMessageHandler(const TSharedRef<FGenericApplicationMessageHandler>& handler)
    {
        messageHandler = handler;
    }

    void Window::Initialize(const TSharedRef<FGenericWindowDefinition>& definition, bool show)
    {
        if (destroyed || Definition) throw std::runtime_error("Game window cannot be initialized again");
        if (definition->Type != EWindowType::Normal || definition->TransparencySupport != EWindowTransparency::None || definition->Opacity != 1 || definition->IsModalWindow) throw std::runtime_error("Only an opaque primary game window is implemented");
        if (!std::isfinite(definition->WidthDesiredOnScreen) || !std::isfinite(definition->HeightDesiredOnScreen) || definition->WidthDesiredOnScreen < 1 || definition->HeightDesiredOnScreen < 1 || definition->WidthDesiredOnScreen > 16384 || definition->HeightDesiredOnScreen > 16384) throw std::invalid_argument("Invalid game window dimensions");
        width = static_cast<int32>(definition->WidthDesiredOnScreen);
        height = static_cast<int32>(definition->HeightDesiredOnScreen);
        Definition = definition;
        visible->store(show, std::memory_order_release);
    }

    void Window::requireInitialized() const
    {
        if (destroyed || !Definition || !output) throw std::runtime_error("Game window is not initialized or has been destroyed");
    }

    std::shared_ptr<Video::Output> Window::GetOutput() const { requireInitialized(); return output; }
    std::shared_ptr<const std::atomic<bool>> Window::CaptureVisibility() const { requireInitialized(); return visible; }
    bool Window::IsDestroyed() const { return destroyed; }

    void Window::ReshapeWindow(int32 x, int32 y, int32 nextWidth, int32 nextHeight)
    {
        requireInitialized();
        if (x != 0 || y != 0) throw std::runtime_error("Positioning the game output is not implemented");
        if (nextWidth < 1 || nextHeight < 1 || nextWidth > 16384 || nextHeight > 16384) throw std::invalid_argument("Invalid game window dimensions");
        const auto handler = messageHandler.Pin();
        if (!handler.IsValid()) throw std::runtime_error("Game window message handler has expired");
        width = nextWidth;
        height = nextHeight;
        handler->OnSizeChanged(AsShared(), width, height);
    }

    bool Window::GetFullScreenInfo(int32& x, int32& y, int32& fullWidth, int32& fullHeight) const
    {
        requireInitialized();
        const auto resolution = output->GetResolution();
        if (resolution.fullWidth > std::numeric_limits<int32>::max() || resolution.fullHeight > std::numeric_limits<int32>::max()) throw std::overflow_error("Video resolution exceeds window coordinates");
        x = 0;
        y = 0;
        fullWidth = static_cast<int32>(resolution.fullWidth);
        fullHeight = static_cast<int32>(resolution.fullHeight);
        return true;
    }

    void Window::MoveWindowTo(int32 x, int32 y)
    {
        requireInitialized();
        if (x != 0 || y != 0) throw std::runtime_error("Positioning the game output is not implemented");
    }

    void Window::BringToFront(bool) { SetWindowFocus(); }
    void Window::HACK_ForceToFront() { throw std::runtime_error("Forcing application focus is not implemented"); }
    void Window::Minimize() { throw std::runtime_error("Minimizing the game output is not implemented"); }
    void Window::Maximize() { throw std::runtime_error("Maximizing the game output is not implemented"); }
    void Window::Restore() { throw std::runtime_error("Restoring the game output is not implemented"); }
    void Window::Show() { requireInitialized(); visible->store(true, std::memory_order_release); }
    void Window::Hide() { throw std::runtime_error("Hiding the game output is not implemented"); }

    void Window::Destroy()
    {
        if (destroyed) return;
        if (output && output.use_count() == 1) output->Close();
        output.reset();
        visible->store(false, std::memory_order_release);
        destroyed = true;
    }

    EWindowMode::Type Window::GetWindowMode() const
    {
        requireInitialized();
        return ANYUNREAL_TARGET_PS5 ? EWindowMode::Fullscreen : EWindowMode::Windowed;
    }

    void Window::SetWindowMode(EWindowMode::Type mode)
    {
        if (mode != GetWindowMode()) throw std::runtime_error("Changing the native game window mode is not implemented");
    }

    bool Window::IsMaximized() const { return false; }
    bool Window::IsMinimized() const { return false; }
    bool Window::IsVisible() const { return visible->load(std::memory_order_acquire); }

    bool Window::GetRestoredDimensions(int32& x, int32& y, int32& restoredWidth, int32& restoredHeight)
    {
        requireInitialized();
        x = 0;
        y = 0;
        restoredWidth = width;
        restoredHeight = height;
        return true;
    }

    void Window::SetWindowFocus()
    {
        requireInitialized();
        if (!IsForegroundWindow()) throw std::runtime_error("Activating a background application is not implemented");
    }

    void Window::SetOpacity(float opacity)
    {
        requireInitialized();
        if (opacity != 1) throw std::runtime_error("Transparent game output is not implemented");
    }

    void Window::Enable(bool enabled)
    {
        requireInitialized();
        Definition->AcceptsInput = enabled;
    }

    bool Window::IsPointInWindow(int32 x, int32 y) const { return IsVisible() && x >= 0 && y >= 0 && x < width && y < height; }
    int32 Window::GetWindowBorderSize() const { return 0; }
    int32 Window::GetWindowTitleBarSize() const { return 0; }
    void* Window::GetOSWindowHandle() const { requireInitialized(); return const_cast<Window*>(this); }
    bool Window::IsForegroundWindow() const { return IsVisible() && FAnyUnrealPlatformApplicationMisc::IsThisApplicationForeground(); }
    bool Window::SupportsDynamicCursorLock() const { return false; }
    bool Window::IsFullscreenSupported() const { return ANYUNREAL_TARGET_PS5; }

    void Window::SetText(const TCHAR* const text)
    {
        requireInitialized();
        if (!text) throw std::invalid_argument("Window title is null");
        if (Definition->Title != text) throw std::runtime_error("Changing the native game window title is not implemented");
    }
}
