#ifndef ANYUNREAL_WINDOWING_APPLICATION_H
#define ANYUNREAL_WINDOWING_APPLICATION_H

#include "CoreMinimal.h"
#include "GenericPlatform/GenericApplication.h"
#include "AnyUnreal/Windowing/Window.h"

namespace AnyUnreal::Windowing
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

    private:
        std::shared_ptr<Video::Output> output;
        TSharedPtr<Window> window;
    };
}

#endif
