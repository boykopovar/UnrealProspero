#include "AnyUnreal/Windowing/Application.h"
#include <limits>
#include <stdexcept>

namespace AnyUnreal::Windowing
{
    Application::Application() : GenericApplication(nullptr), output(std::make_shared<Video::Output>()) {}

    void Application::SetMessageHandler(const TSharedRef<FGenericApplicationMessageHandler>& handler)
    {
        GenericApplication::SetMessageHandler(handler);
        if (window.IsValid()) window->SetMessageHandler(handler);
    }

    TSharedRef<FGenericWindow> Application::MakeWindow()
    {
        if (!output) throw std::runtime_error("Application has been destroyed");
        if (window.IsValid() && !window->IsDestroyed()) throw std::runtime_error("Multiple game windows are not implemented");
        window = MakeShared<Window>(output, GetMessageHandler());
        return window.ToSharedRef();
    }

    void Application::InitializeWindow(const TSharedRef<FGenericWindow>& nextWindow, const TSharedRef<FGenericWindowDefinition>& definition, const TSharedPtr<FGenericWindow>& parent, bool showImmediately)
    {
        if (!window.IsValid() || &nextWindow.Get() != window.Get()) throw std::invalid_argument("Window does not belong to this application");
        if (parent.IsValid()) throw std::runtime_error("Child game windows are not implemented");
        window->Initialize(definition, showImmediately);
    }

    FPlatformRect Application::GetWorkArea(const FPlatformRect&) const
    {
        if (!output) throw std::runtime_error("Application has been destroyed");
        const auto resolution = output->GetResolution();
        if (resolution.paneWidth > std::numeric_limits<int32>::max() || resolution.paneHeight > std::numeric_limits<int32>::max()) throw std::overflow_error("Video work area exceeds window coordinates");
        return {0, 0, static_cast<int32>(resolution.paneWidth), static_cast<int32>(resolution.paneHeight)};
    }

    void Application::GetInitialDisplayMetrics(FDisplayMetrics& metrics) const
    {
        if (!output) throw std::runtime_error("Application has been destroyed");
        const auto resolution = output->GetResolution();
        if (resolution.fullWidth > std::numeric_limits<int32>::max() || resolution.fullHeight > std::numeric_limits<int32>::max()) throw std::overflow_error("Video resolution exceeds window coordinates");
        FDisplayMetrics result;
        result.PrimaryDisplayWidth = static_cast<int32>(resolution.fullWidth);
        result.PrimaryDisplayHeight = static_cast<int32>(resolution.fullHeight);
        result.VirtualDisplayRect = {0, 0, result.PrimaryDisplayWidth, result.PrimaryDisplayHeight};
        result.PrimaryDisplayWorkAreaRect = GetWorkArea(result.VirtualDisplayRect);
        metrics = MoveTemp(result);
    }

    void Application::DestroyApplication()
    {
        if (window.IsValid()) window->Destroy();
        window.Reset();
        if (output && output.use_count() == 1) output->Close();
        output.reset();
    }
}
