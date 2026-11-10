#include "Containers/LockFreeList.h"
#include "AnyUnreal/AnyUnrealRunnableThread.h"
#include "HAL/RunnableThread.h"
#include "HAL/PlatformFile.h"
#include "CoreTypes.h"
#include "AnyUnreal/File/FileHandle.h"
#include "AnyUnreal/AnyUnrealEvent.h"
#include "AnyUnreal/AnyUnrealPlatformProperties.h"
#include "HAL/PlatformProperties.h"
#include "GenericPlatform/GenericPlatformTime.h"
#include <cstddef>
#include <string_view>
#include <type_traits>

static_assert(sizeof(TCHAR) == 2);
static_assert(sizeof(UPTRINT) == sizeof(void*));
static_assert(sizeof(PTRINT) == sizeof(void*));
static_assert(std::is_same_v<SIZE_T, std::size_t>);
static_assert(std::is_same_v<SSIZE_T, std::ptrdiff_t>);
static_assert(std::is_same_v<TCHAR, wchar_t>);
static_assert(PLATFORM_WINDOWS == 0);
static_assert(PLATFORM_LINUX == 0);
static_assert(PLATFORM_UNIX == 0);
static_assert(PLATFORM_EXCEPTIONS_DISABLED == 0);

using WindowsProperties = TAnyUnrealPlatformProperties<EAnyUnrealTarget::Windows>;
using LinuxProperties = TAnyUnrealPlatformProperties<EAnyUnrealTarget::Linux>;
using Ps5Properties = TAnyUnrealPlatformProperties<EAnyUnrealTarget::Ps5>;
using ServerProperties = TAnyUnrealPlatformProperties<EAnyUnrealTarget::Linux, true>;
using ClientProperties = TAnyUnrealPlatformProperties<EAnyUnrealTarget::Windows, false, true>;

static_assert(std::string_view(WindowsProperties::PlatformName()) == "AnyWin64");
static_assert(std::string_view(LinuxProperties::PlatformName()) == "AnyLinux");
static_assert(std::string_view(Ps5Properties::PlatformName()) == "AnyPS5");
static_assert(std::string_view(ServerProperties::PlatformName()) == "AnyLinuxServer");
static_assert(std::string_view(ClientProperties::PlatformName()) == "AnyWin64Client");
static_assert(std::string_view(ServerProperties::IniPlatformName()) == "AnyLinux");
static_assert(std::string_view(ClientProperties::IniPlatformName()) == "AnyWin64");
static_assert(WindowsProperties::SupportsWindowedMode());
static_assert(LinuxProperties::SupportsMultipleGameInstances());
static_assert(!Ps5Properties::SupportsWindowedMode());
static_assert(!Ps5Properties::SupportsMultipleGameInstances());
static_assert(Ps5Properties::HasFixedResolution());
static_assert(!WindowsProperties::HasFixedResolution());
static_assert(ServerProperties::IsServerOnly() && !ServerProperties::IsClientOnly());
static_assert(!ServerProperties::SupportsWindowedMode());
static_assert(ClientProperties::IsClientOnly() && !ClientProperties::IsServerOnly());
static_assert(FPlatformProperties::RequiresCookedData());
static_assert(!FPlatformProperties::HasEditorOnlyData());
static_assert(!FPlatformProperties::HasSecurePackageFormat());
static_assert(FPlatformProperties::SupportsBuildTarget(EBuildTargetType::Game));
static_assert(FPlatformProperties::SupportsBuildTarget(EBuildTargetType::Client));
static_assert(FPlatformProperties::SupportsBuildTarget(EBuildTargetType::Server));
static_assert(FPlatformProperties::SupportsBuildTarget(EBuildTargetType::Program));
static_assert(!FPlatformProperties::SupportsBuildTarget(EBuildTargetType::Editor));
static_assert(!FPlatformProperties::SupportsBuildTarget(EBuildTargetType::Unknown));
static_assert(FPlatformProperties::GetMaxSupportedVirtualMemoryAlignment() == 16384);

#if ANYUNREAL_TARGET_WINDOWS
static_assert(sizeof(long) == 4);
static_assert(std::string_view(FPlatformProperties::IniPlatformName()) == "AnyWin64");
#elif ANYUNREAL_TARGET_LINUX
static_assert(sizeof(long) == 8);
static_assert(std::string_view(FPlatformProperties::IniPlatformName()) == "AnyLinux");
#else
static_assert(sizeof(long) == 8);
static_assert(std::string_view(FPlatformProperties::IniPlatformName()) == "AnyPS5");
#endif

static_assert(std::is_base_of_v<FEvent, FAnyUnrealEvent>);
static_assert(!std::is_abstract_v<FAnyUnrealEvent>);
static_assert(!std::is_copy_constructible_v<FAnyUnrealEvent>);
static_assert(!std::is_move_constructible_v<FAnyUnrealEvent>);
static_assert(std::is_nothrow_destructible_v<FAnyUnrealEvent>);

static_assert(std::is_base_of_v<IFileHandle, FAnyUnrealFileHandle>);
static_assert(!std::is_abstract_v<FAnyUnrealFileHandle>);
static_assert(!std::is_copy_constructible_v<FAnyUnrealFileHandle>);

static_assert(!std::is_abstract_v<FAnyUnrealPlatformFile>);

static_assert(!std::is_abstract_v<FAnyUnrealRunnableThread>);
static_assert(alignof(FIndexedPointer) >= 8);
