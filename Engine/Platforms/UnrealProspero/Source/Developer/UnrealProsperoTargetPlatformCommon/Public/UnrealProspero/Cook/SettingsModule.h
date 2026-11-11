#ifndef UNREALPROSPERO_COOK_SETTINGS_MODULE_H
#define UNREALPROSPERO_COOK_SETTINGS_MODULE_H

#include "UnrealProspero/Cook/Settings.h"
#include "Interfaces/ITargetPlatformSettingsModule.h"

namespace UnrealProspero::Cook
{
    template<EUnrealProsperoTarget TTarget>
    class SettingsModule final : public ITargetPlatformSettingsModule
    {
    public:
        ITargetPlatformSettings* GetGameSettings()
        {
            ITargetPlatformSettingsModule::GetTargetPlatformSettings();
            return gameSettings;
        }

        ITargetPlatformSettings* GetServerSettings()
        {
            ITargetPlatformSettingsModule::GetTargetPlatformSettings();
            return serverSettings;
        }

        ITargetPlatformSettings* GetClientSettings()
        {
            ITargetPlatformSettingsModule::GetTargetPlatformSettings();
            return clientSettings;
        }

    protected:
        void GetTargetPlatformSettings(TArray<ITargetPlatformSettings*>& platforms) override
        {
            gameSettings = new Settings<TUnrealProsperoPlatformProperties<TTarget>>();
            platforms.Add(gameSettings);
            serverSettings = new Settings<TUnrealProsperoPlatformProperties<TTarget, true>>();
            platforms.Add(serverSettings);
            clientSettings = new Settings<TUnrealProsperoPlatformProperties<TTarget, false, true>>();
            platforms.Add(clientSettings);
        }

    private:
        ITargetPlatformSettings* gameSettings = nullptr;
        ITargetPlatformSettings* serverSettings = nullptr;
        ITargetPlatformSettings* clientSettings = nullptr;
    };
}

#endif
