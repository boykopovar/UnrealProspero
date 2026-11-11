#ifndef UNREALPROSPERO_WINDOWING_APPLICATION_H
#define UNREALPROSPERO_WINDOWING_APPLICATION_H

#include "CoreMinimal.h"
#include "GenericPlatform/GenericApplication.h"
#include "UnrealProspero/Windowing/Window.h"
#include "UnrealProspero/Input/ControllerEvents.h"

namespace UnrealProspero::Windowing
{
    class APPLICATIONCORE_API Application final : public GenericApplication
    {
    public:
        Application();
        void SetMessageHandler(const TSharedRef<FGenericApplicationMessageHandler>& handler) override;
        TSharedRef<FGenericWindow> MakeWindow() override;
        void InitializeWindow(const TSharedRef<FGenericWindow>& window, const TSharedRef<FGenericWindowDefinition>& definition, const TSharedPtr<FGenericWindow>& parent, bool showImmediately) override;
        FPlatformRect GetWorkArea(const FPlatformRect& currentWindow) const override;
        void GetInitialDisplayMetrics(FDisplayMetrics& metrics) const override;
        void DestroyApplication() override;
        void PollGameDeviceState(float timeDelta) override;
        bool IsGamepadAttached() const override;

    private:
        std::shared_ptr<Video::Output> output;
        TSharedPtr<Window> window;
        std::unique_ptr<Input::ControllerHub> controllers;
        std::unique_ptr<Input::ControllerEvents> controllerEvents;
    };
}

#endif
