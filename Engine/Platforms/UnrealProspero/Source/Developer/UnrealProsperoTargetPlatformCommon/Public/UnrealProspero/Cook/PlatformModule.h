#ifndef UNREALPROSPERO_COOK_PLATFORM_MODULE_H
#define UNREALPROSPERO_COOK_PLATFORM_MODULE_H

#include "Common/TargetPlatformBase.h"
#include "Interfaces/ITargetPlatformModule.h"
#include <stdexcept>

namespace UnrealProspero::Cook
{
    class PlatformModule final : public ITargetPlatformModule
    {
    protected:
        void GetTargetPlatforms(TArray<ITargetPlatform*>&) override
        {
            throw std::logic_error("Target platform settings and controls are required");
        }

        void GetTargetPlatforms(TArray<ITargetPlatform*>& platforms, TArray<ITargetPlatformSettings*> settings, TArray<ITargetPlatformControls*> controls) override
        {
            if (settings.IsEmpty() || settings.Num() != controls.Num()) throw std::invalid_argument("Target platform settings and controls do not match");
            TArray<ITargetPlatformSettings*> matchedSettings;
            for (ITargetPlatformControls* control : controls)
            {
                if (!control) throw std::invalid_argument("Target platform controls are null");
                ITargetPlatformSettings* platformSettings = control->GetTargetPlatformSettings();
                if (!platformSettings || !settings.Contains(platformSettings) || matchedSettings.Contains(platformSettings)) throw std::invalid_argument("Target platform controls have invalid settings");
                matchedSettings.Add(platformSettings);
            }
            for (ITargetPlatformControls* control : controls) platforms.Add(new FTargetPlatformMerged(control->GetTargetPlatformSettings(), control));
        }
    };
}

#endif
