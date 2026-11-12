#include "PlatformFeatures.h"
#include "UnrealProspero/SaveGameSystem.h"

class FUnrealProsperoPlatformFeatures final : public IPlatformFeaturesModule
{
public:
    ISaveGameSystem* GetSaveGameSystem() override
    {
        static UnrealProspero::SaveGameSystem system;
        return &system;
    }
};

IMPLEMENT_MODULE(FUnrealProsperoPlatformFeatures, UnrealProsperoPlatformFeatures)
