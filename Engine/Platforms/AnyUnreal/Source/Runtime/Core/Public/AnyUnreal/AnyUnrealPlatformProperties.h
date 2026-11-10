#ifndef ANYUNREAL_PLATFORM_PROPERTIES_H
#define ANYUNREAL_PLATFORM_PROPERTIES_H

#include "GenericPlatform/GenericPlatformProperties.h"
#include "AnyUnreal/Kernel/DirectAllocation.h"

enum class EAnyUnrealTarget
{
    Windows,
    Linux,
    Ps5
};

template<EAnyUnrealTarget TTarget, bool TServer = false, bool TClient = false>
struct TAnyUnrealPlatformProperties : FGenericPlatformProperties
{
    static_assert(TTarget == EAnyUnrealTarget::Windows || TTarget == EAnyUnrealTarget::Linux || TTarget == EAnyUnrealTarget::Ps5);
    static_assert(!(TServer && TClient));

    static constexpr const char* IniPlatformName()
    {
        if constexpr (TTarget == EAnyUnrealTarget::Windows) return "AnyWin64";
        if constexpr (TTarget == EAnyUnrealTarget::Linux) return "AnyLinux";
        return "AnyPS5";
    }

    static constexpr const char* PlatformName()
    {
        return IniPlatformName();
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
        return TTarget != EAnyUnrealTarget::Ps5;
    }

    static constexpr bool SupportsWindowedMode()
    {
        return TTarget != EAnyUnrealTarget::Ps5 && !TServer;
    }

    static constexpr bool HasFixedResolution()
    {
        return TTarget == EAnyUnrealTarget::Ps5;
    }

    static constexpr bool SupportsQuit()
    {
        return true;
    }

    static constexpr int GetMaxSupportedVirtualMemoryAlignment()
    {
        return AnyUnreal::Kernel::DirectAllocation::PageSize;
    }
};

#endif

#if defined(PROPERTY_HEADER_SHOULD_DEFINE_TYPE) && !defined(ANYUNREAL_PLATFORM_PROPERTIES_TYPE_DEFINED)
#define ANYUNREAL_PLATFORM_PROPERTIES_TYPE_DEFINED

#if ANYUNREAL_TARGET_WINDOWS
using FPlatformProperties = TAnyUnrealPlatformProperties<EAnyUnrealTarget::Windows, UE_SERVER, !WITH_SERVER_CODE && !WITH_EDITOR>;
#elif ANYUNREAL_TARGET_LINUX
using FPlatformProperties = TAnyUnrealPlatformProperties<EAnyUnrealTarget::Linux, UE_SERVER, !WITH_SERVER_CODE && !WITH_EDITOR>;
#elif ANYUNREAL_TARGET_PS5
using FPlatformProperties = TAnyUnrealPlatformProperties<EAnyUnrealTarget::Ps5, UE_SERVER, !WITH_SERVER_CODE && !WITH_EDITOR>;
#else
#error An AnyUnreal target must be selected for runtime platform properties
#endif

#endif
