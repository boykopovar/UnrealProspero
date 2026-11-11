#ifndef UNREALPROSPERO_COOK_SETTINGS_MODULE_H
#define UNREALPROSPERO_COOK_SETTINGS_MODULE_H

#include "UnrealProspero/Cook/Settings.h"
#include "Interfaces/ITargetPlatformSettingsModule.h"

namespace UnrealProspero::Cook
{
    template<EUnrealProsperoTarget TTarget>
    class SettingsModule final : public ITargetPlatformSettingsModule
    {
    protected:
        void GetTargetPlatformSettings(TArray<ITargetPlatformSettings*>& platforms) override
        {
            platforms.Add(new Settings<TUnrealProsperoPlatformProperties<TTarget>>());
            platforms.Add(new Settings<TUnrealProsperoPlatformProperties<TTarget, true>>());
            platforms.Add(new Settings<TUnrealProsperoPlatformProperties<TTarget, false, true>>());
        }
    };
}

#endif
