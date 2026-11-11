#ifndef UNREALPROSPERO_COOK_SETTINGS_H
#define UNREALPROSPERO_COOK_SETTINGS_H

#include "Common/TargetPlatformSettingsBase.h"
#include "UnrealProspero/UnrealProsperoPlatformProperties.h"
#include <stdexcept>
#if WITH_ENGINE
#include "StaticMeshResources.h"
#endif

namespace UnrealProspero::Cook
{
    template<typename TProperties>
    class Settings final : public TTargetPlatformSettingsBase<TProperties>
    {
    public:
        Settings()
        {
#if WITH_ENGINE
            meshLodSettings.Initialize(this);
#endif
        }

        void GetAllPossibleShaderFormats(TArray<FName>& formats) const override
        {
            if constexpr (!TProperties::IsServerOnly()) formats.AddUnique(FName(*(FString(TEXT("SF_AGC_")) + FString(TProperties::IniPlatformName()).ToUpper())));
        }

        void GetAllTargetedShaderFormats(TArray<FName>& formats) const override
        {
            GetAllPossibleShaderFormats(formats);
        }

        bool SupportsFeature(ETargetPlatformFeatures feature) const override
        {
            if (feature == ETargetPlatformFeatures::TextureStreaming) return false;
            if (feature == ETargetPlatformFeatures::Packaging) return true;
            return TTargetPlatformSettingsBase<TProperties>::SupportsFeature(feature);
        }

        bool UsesRayTracing() const override { return false; }
        ERayTracingRuntimeMode GetRayTracingMode() const override { return ERayTracingRuntimeMode::Disabled; }
        EOfflineBVHMode GetStaticMeshOfflineBVHMode() const override { return EOfflineBVHMode::Disabled; }
        EOfflineBVHMode GetSkeletalMeshOfflineBVHMode() const override { return EOfflineBVHMode::Disabled; }
        bool GetStaticMeshOfflineBVHCompression() const override { return false; }

        void GetRayTracingShaderFormats(TArray<FName>&) const override
        {
        }

        void GetPossibleArchitectures(TArray<FString>& architectures) const override
        {
            architectures.AddUnique(TEXT("x64"));
        }

#if WITH_ENGINE
        void GetShaderFormatModuleHints(TArray<FName>& modules) const override
        {
            if constexpr (!TProperties::IsServerOnly()) modules.AddUnique(TEXT("UnrealProsperoShaderFormat"));
        }

        const UTextureLODSettings& GetTextureLODSettings() const override
        {
            if (!textureLodSettings) throw std::runtime_error("Texture LOD settings have not been registered");
            return *textureLodSettings;
        }

        void RegisterTextureLODSettings(const UTextureLODSettings* settings) override
        {
            if (!settings) throw std::invalid_argument("Texture LOD settings are null");
            textureLodSettings = settings;
        }

        const FStaticMeshLODSettings& GetStaticMeshLODSettings() const override
        {
            return meshLodSettings;
        }
#endif

    private:
#if WITH_ENGINE
        const UTextureLODSettings* textureLodSettings = nullptr;
        FStaticMeshLODSettings meshLodSettings;
#endif
    };
}

#endif
