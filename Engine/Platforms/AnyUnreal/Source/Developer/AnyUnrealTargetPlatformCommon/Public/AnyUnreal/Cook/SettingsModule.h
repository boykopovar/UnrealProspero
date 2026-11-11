#ifndef ANYUNREAL_COOK_SETTINGS_MODULE_H
#define ANYUNREAL_COOK_SETTINGS_MODULE_H

#include "AnyUnreal/Cook/Settings.h"
#include "Interfaces/ITargetPlatformSettingsModule.h"

namespace AnyUnreal::Cook
{
    template<EAnyUnrealTarget TTarget>
    class SettingsModule final : public ITargetPlatformSettingsModule
    {
    protected:
        void GetTargetPlatformSettings(TArray<ITargetPlatformSettings*>& platforms) override
        {
            platforms.Add(new Settings<TAnyUnrealPlatformProperties<TTarget>>());
            platforms.Add(new Settings<TAnyUnrealPlatformProperties<TTarget, true>>());
            platforms.Add(new Settings<TAnyUnrealPlatformProperties<TTarget, false, true>>());
        }
    };
}

#endif
