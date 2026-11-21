#ifndef UNREALPROSPERO_PLATFORM_PROPERTIES_H
#define UNREALPROSPERO_PLATFORM_PROPERTIES_H

#include "GenericPlatform/GenericPlatformProperties.h"
#include "UnrealProspero/Kernel/MemoryConstants.h"
#include <array>
#include <string_view>

enum class EUnrealProsperoTarget
{
    Windows,
    Linux,
    Ps5
};

template<EUnrealProsperoTarget TTarget, bool TServer = false, bool TClient = false>
struct TUnrealProsperoPlatformProperties : FGenericPlatformProperties
{
    static_assert(TTarget == EUnrealProsperoTarget::Windows || TTarget == EUnrealProsperoTarget::Linux || TTarget == EUnrealProsperoTarget::Ps5);
    static_assert(!(TServer && TClient));

    static constexpr EUnrealProsperoTarget GetTarget()
    {
        return TTarget;
    }

    static constexpr const char* IniPlatformName()
    {
        if constexpr (TTarget == EUnrealProsperoTarget::Windows) return "AnyWin64";
        else if constexpr (TTarget == EUnrealProsperoTarget::Linux) return "AnyLinux";
        else return "AnyPS5";
    }

    static constexpr const char* PlatformName()
    {
        return platformName.data();
    }

    static constexpr const TCHAR* GetRuntimeSettingsClassName()
    {
        return TEXT("UnrealProsperoRuntimeSettings");
    }

    static constexpr bool HasEditorOnlyData()
    {
        return false;
    }

    static constexpr bool RequiresCookedData()
    {
        return true;
    }

    static constexpr bool IsServerOnly()
    {
        return TServer;
    }

    static constexpr bool IsClientOnly()
    {
        return TClient;
    }

    static constexpr bool SupportsBuildTarget(EBuildTargetType target)
    {
        return target == EBuildTargetType::Game || target == EBuildTargetType::Client || target == EBuildTargetType::Server || target == EBuildTargetType::Program;
    }

    static constexpr bool SupportsMultipleGameInstances()
    {
        return TTarget != EUnrealProsperoTarget::Ps5;
    }

    static constexpr bool SupportsWindowedMode()
    {
        return TTarget != EUnrealProsperoTarget::Ps5 && !TServer;
    }

    static constexpr bool HasFixedResolution()
    {
        return false;
    }

    static constexpr bool SupportsQuit()
    {
        return true;
    }

    static constexpr int GetMaxSupportedVirtualMemoryAlignment()
    {
        return UnrealProspero::Kernel::MemoryPageSize;
    }

private:
    inline static constexpr auto platformName = []
    {
        constexpr std::string_view name = IniPlatformName();
        constexpr std::string_view suffix = TServer ? "Server" : TClient ? "Client" : "";
        std::array<char, name.size() + suffix.size() + 1> result{};
        for (std::size_t index = 0; index < name.size(); ++index) result[index] = name[index];
        for (std::size_t index = 0; index < suffix.size(); ++index) result[name.size() + index] = suffix[index];
        return result;
    }();
};

#endif

#if defined(PROPERTY_HEADER_SHOULD_DEFINE_TYPE) && !defined(UNREALPROSPERO_PLATFORM_PROPERTIES_TYPE_DEFINED)
#define UNREALPROSPERO_PLATFORM_PROPERTIES_TYPE_DEFINED

#if UNREALPROSPERO_TARGET_WINDOWS
using FPlatformProperties = TUnrealProsperoPlatformProperties<EUnrealProsperoTarget::Windows, UE_SERVER, !WITH_SERVER_CODE && !WITH_EDITOR>;
#elif UNREALPROSPERO_TARGET_LINUX
using FPlatformProperties = TUnrealProsperoPlatformProperties<EUnrealProsperoTarget::Linux, UE_SERVER, !WITH_SERVER_CODE && !WITH_EDITOR>;
#elif UNREALPROSPERO_TARGET_PS5
using FPlatformProperties = TUnrealProsperoPlatformProperties<EUnrealProsperoTarget::Ps5, UE_SERVER, !WITH_SERVER_CODE && !WITH_EDITOR>;
#else
#error An UnrealProspero target must be selected for runtime platform properties
#endif

#endif
