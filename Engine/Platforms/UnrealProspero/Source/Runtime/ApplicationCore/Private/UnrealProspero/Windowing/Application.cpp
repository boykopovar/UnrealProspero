#include "UnrealProspero/Windowing/Application.h"
#include <limits>
#include <stdexcept>

namespace UnrealProspero::Windowing
{
    static FPlatformRect WorkArea(const VideoOutResolutionStatus& resolution)
    {
        if (resolution.paneWidth > std::numeric_limits<int32>::max() || resolution.paneHeight > std::numeric_limits<int32>::max()) throw std::overflow_error("Video work area exceeds window coordinates");
        return {0, 0, static_cast<int32>(resolution.paneWidth), static_cast<int32>(resolution.paneHeight)};
    }

    static void ReadDisplayMetrics(const Video::Output& output, FDisplayMetrics& metrics)
    {
        const auto resolution = output.GetResolution();
        if (resolution.fullWidth > std::numeric_limits<int32>::max() || resolution.fullHeight > std::numeric_limits<int32>::max()) throw std::overflow_error("Video resolution exceeds window coordinates");
        FDisplayMetrics result;
        result.PrimaryDisplayWidth = static_cast<int32>(resolution.fullWidth);
        result.PrimaryDisplayHeight = static_cast<int32>(resolution.fullHeight);
        result.VirtualDisplayRect = {0, 0, result.PrimaryDisplayWidth, result.PrimaryDisplayHeight};
        result.PrimaryDisplayWorkAreaRect = WorkArea(resolution);
        metrics = MoveTemp(result);
    }

    Application::Application() : GenericApplication(nullptr), output(Video::Output::AcquirePrimary()), controllers(std::make_unique<Input::ControllerHub>()), controllerEvents(std::make_unique<Input::ControllerEvents>(controllers->GetInitialUser())) {}

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
        return WorkArea(output->GetResolution());
    }

    void Application::GetInitialDisplayMetrics(FDisplayMetrics& metrics) const
    {
        if (!output) throw std::runtime_error("Application has been destroyed");
        ReadDisplayMetrics(*output, metrics);
    }

    void Application::PollGameDeviceState(float deltaTime)
    {
        if (!controllers) throw std::runtime_error("Application has been destroyed");
        const bool enabled = window.IsValid() && !window->IsDestroyed() && window->IsVisible() && window->GetDefinition().AcceptsInput && window->IsForegroundWindow();
        controllerEvents->Dispatch(controllers->Poll(), GetMessageHandler().Get(), enabled, deltaTime);
    }

    bool Application::IsGamepadAttached() const
    {
        return controllerEvents && controllerEvents->IsAttached();
    }

    void Application::DestroyApplication()
    {
        if (controllerEvents) controllerEvents->Dispatch({}, GetMessageHandler().Get(), false, 0);
        if (controllers) controllers->Close();
        controllerEvents.reset();
        controllers.reset();
        if (window.IsValid()) window->Destroy();
        window.Reset();
        if (output && output.use_count() == 1) output->Close();
        output.reset();
    }
}

void FDisplayMetrics::RebuildDisplayMetrics(FDisplayMetrics& metrics)
{
    UnrealProspero::Windowing::ReadDisplayMetrics(*UnrealProspero::Video::Output::AcquirePrimary(), metrics);
}
