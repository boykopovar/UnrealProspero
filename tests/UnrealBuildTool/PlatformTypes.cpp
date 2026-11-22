#include "Containers/LockFreeList.h"
#include "UnrealProspero/UnrealProsperoRunnableThread.h"
#include "HAL/RunnableThread.h"
#include "HAL/PlatformFile.h"
#include "CoreTypes.h"
#include "UnrealProspero/File/FileHandle.h"
#include "UnrealProspero/UnrealProsperoEvent.h"
#include "UnrealProspero/UnrealProsperoPlatformProperties.h"
#include "HAL/PlatformProperties.h"
#include "GenericPlatform/GenericPlatformTime.h"
#include <cstddef>
#include <string_view>
#include <type_traits>

static_assert(sizeof(TCHAR) == 2);
static_assert(sizeof(UPTRINT) == sizeof(void*));
static_assert(sizeof(PTRINT) == sizeof(void*));
static_assert(std::is_same_v<SIZE_T, uint64>);
static_assert(std::is_same_v<SSIZE_T, int64>);
static_assert(sizeof(SSIZE_T) == sizeof(void*));
static_assert(PLATFORM_USE_PTHREADS == 0);
static_assert(std::is_same_v<TCHAR, wchar_t>);
static_assert(PLATFORM_WINDOWS == 0);
static_assert(PLATFORM_LINUX == 0);
static_assert(PLATFORM_UNIX == 0);
static_assert(PLATFORM_EXCEPTIONS_DISABLED == 0);

using Ps5Properties = TUnrealProsperoPlatformProperties<EUnrealProsperoTarget::Ps5>;
using ServerProperties = TUnrealProsperoPlatformProperties<EUnrealProsperoTarget::Ps5, true>;
using ClientProperties = TUnrealProsperoPlatformProperties<EUnrealProsperoTarget::Ps5, false, true>;

static_assert(std::string_view(Ps5Properties::PlatformName()) == "AnyPS5");
static_assert(std::string_view(ServerProperties::PlatformName()) == "AnyPS5Server");
static_assert(std::string_view(ClientProperties::PlatformName()) == "AnyPS5Client");
static_assert(std::string_view(ServerProperties::IniPlatformName()) == "AnyPS5");
static_assert(std::string_view(ClientProperties::IniPlatformName()) == "AnyPS5");
static_assert(!Ps5Properties::SupportsWindowedMode());
static_assert(!Ps5Properties::SupportsMultipleGameInstances());
static_assert(!Ps5Properties::HasFixedResolution());
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

static_assert(sizeof(long) == 8);
static_assert(std::string_view(FPlatformProperties::IniPlatformName()) == "AnyPS5");

static_assert(std::is_base_of_v<FEvent, FUnrealProsperoEvent>);
static_assert(!std::is_abstract_v<FUnrealProsperoEvent>);
static_assert(!std::is_copy_constructible_v<FUnrealProsperoEvent>);
static_assert(!std::is_move_constructible_v<FUnrealProsperoEvent>);
static_assert(std::is_nothrow_destructible_v<FUnrealProsperoEvent>);

static_assert(std::is_base_of_v<IFileHandle, FUnrealProsperoFileHandle>);
static_assert(!std::is_abstract_v<FUnrealProsperoFileHandle>);
static_assert(!std::is_copy_constructible_v<FUnrealProsperoFileHandle>);

static_assert(!std::is_abstract_v<FUnrealProsperoPlatformFile>);

static_assert(!std::is_abstract_v<FUnrealProsperoRunnableThread>);
static_assert(alignof(FIndexedPointer) >= 8);
static_assert(std::is_nothrow_destructible_v<UE::FUnrealProsperoRecursiveMutex>);
static_assert(std::is_nothrow_destructible_v<UE::FUnrealProsperoSharedMutex>);

static_assert(UNREALPROSPERO_TARGET_WINDOWS + UNREALPROSPERO_TARGET_LINUX + UNREALPROSPERO_TARGET_PS5 == 1);
static_assert(PLATFORM_DESKTOP == (UNREALPROSPERO_TARGET_WINDOWS || UNREALPROSPERO_TARGET_LINUX));
