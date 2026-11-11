#ifndef UNREALPROSPERO_COOK_CONTROLS_MODULE_H
#define UNREALPROSPERO_COOK_CONTROLS_MODULE_H

#include "UnrealProspero/Cook/Controls.h"
#include "UnrealProspero/Cook/SettingsModule.h"
#include "Interfaces/ITargetPlatformControlsModule.h"
#include "Modules/ModuleManager.h"

namespace UnrealProspero::Cook
{
    template<EUnrealProsperoTarget TTarget>
    class ControlsModule final : public ITargetPlatformControlsModule
    {
    protected:
        void GetTargetPlatformControls(TArray<ITargetPlatformControls*>& platforms, FName& settingsModuleName) override
        {
            FString expected = FString(TUnrealProsperoPlatformProperties<TTarget>::IniPlatformName()) + TEXT("TargetPlatformSettings");
            if (settingsModuleName != FName(*expected)) throw std::invalid_argument("Target platform settings module does not match controls");
            auto* settings = FModuleManager::GetModulePtr<SettingsModule<TTarget>>(settingsModuleName);
            if (!settings) throw std::runtime_error("Target platform settings module is not loaded");
            platforms.Add(new Controls<TUnrealProsperoPlatformProperties<TTarget>>(settings->GetGameSettings()));
            platforms.Add(new Controls<TUnrealProsperoPlatformProperties<TTarget, true>>(settings->GetServerSettings()));
            platforms.Add(new Controls<TUnrealProsperoPlatformProperties<TTarget, false, true>>(settings->GetClientSettings()));
        }
    };
}

#endif
