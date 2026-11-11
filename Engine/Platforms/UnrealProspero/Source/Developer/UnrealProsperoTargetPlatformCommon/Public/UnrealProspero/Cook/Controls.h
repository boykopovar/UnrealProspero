#ifndef UNREALPROSPERO_COOK_CONTROLS_H
#define UNREALPROSPERO_COOK_CONTROLS_H

#include "Common/TargetPlatformControlsBase.h"
#include "UnrealProspero/Cook/LocalDevice.h"
#include "UnrealProspero/Cook/SdkStatus.h"
#include <stdexcept>
#if WITH_ENGINE
#include "TextureResource.h"
#endif

namespace UnrealProspero::Cook
{
    template<typename TProperties>
    class Controls final : public TTargetPlatformControlsBase<TProperties>
    {
    public:
        explicit Controls(ITargetPlatformSettings* settings) : TTargetPlatformControlsBase<TProperties>(requireSettings(settings)), localDevice(LocalDevice::Create(*this, TProperties::GetTarget())) { }

        void EnableDeviceCheck(bool enabled) override
        {
            checkConnection = enabled;
        }

        void GetAllDevices(TArray<ITargetDevicePtr>& devices) const override
        {
            devices.Reset();
            if (hasLocalDevice()) devices.Add(localDevice);
        }

        ITargetDevicePtr GetDefaultDevice() const override
        {
            return hasLocalDevice() ? localDevice : nullptr;
        }

        ITargetDevicePtr GetDevice(const FTargetDeviceId& deviceId) override
        {
            return hasLocalDevice() && localDevice->GetId() == deviceId ? localDevice : nullptr;
        }

        bool IsSdkInstalled(bool, FString& documentationPath) const override
        {
            documentationPath.Empty();
            return SdkStatus::IsInstalled(TProperties::IniPlatformName());
        }

        bool GenerateStreamingInstallManifest(const TMultiMap<FString, int32>&, const TSet<int32>&) const override
        {
            return true;
        }

        bool CanSupportRemoteShaderCompile() const override
        {
            return false;
        }

        bool SupportsVariants() const override
        {
            return true;
        }

#if WITH_ENGINE
        void GetTextureFormats(const UTexture* texture, TArray<TArray<FName>>& formats) const override
        {
            if (!texture) throw std::invalid_argument("Texture is null");
            if constexpr (!TProperties::IsServerOnly()) GetDefaultTextureFormatNamePerLayer(formats.AddDefaulted_GetRef(), this->GetTargetPlatformSettings(), this, texture, true, 4, true);
        }
#endif

    private:
        static ITargetPlatformSettings* requireSettings(ITargetPlatformSettings* settings)
        {
            if (!settings || settings->IniPlatformName() != TProperties::IniPlatformName()) throw std::invalid_argument("Target platform settings do not match controls");
            return settings;
        }

        bool hasLocalDevice() const
        {
            return localDevice.IsValid() && (!checkConnection || localDevice->IsConnected());
        }

        ITargetDevicePtr localDevice;
        bool checkConnection = true;
    };
}

#endif
