#ifndef UNREALPROSPERO_PLATFORM_H
#define UNREALPROSPERO_PLATFORM_H

#include "Clang/ClangPlatform.h"
#include "GenericPlatform/GenericPlatform.h"
#include "UnrealProspero/Kernel/Debug.h"
#include <bit>
#include <cstddef>

#ifndef UNREALPROSPERO_TARGET_WINDOWS
#define UNREALPROSPERO_TARGET_WINDOWS 0
#endif
#ifndef UNREALPROSPERO_TARGET_LINUX
#define UNREALPROSPERO_TARGET_LINUX 0
#endif
#ifndef UNREALPROSPERO_TARGET_PS5
#define UNREALPROSPERO_TARGET_PS5 0
#endif

#if UNREALPROSPERO_TARGET_WINDOWS + UNREALPROSPERO_TARGET_LINUX + UNREALPROSPERO_TARGET_PS5 != 1
#error Exactly one UnrealProspero target must be selected
#endif

struct FUnrealProsperoPlatformTypes : FGenericPlatformTypes
{
    using DWORD = unsigned int;
    using SIZE_T = uint64;
    using SSIZE_T = int64;
    using TYPE_OF_NULL = decltype(NULL);
};

using FPlatformTypes = FUnrealProsperoPlatformTypes;

struct tagRECT
{
    FPlatformTypes::int32 left;
    FPlatformTypes::int32 top;
    FPlatformTypes::int32 right;
    FPlatformTypes::int32 bottom;
};

static_assert(sizeof(void*) == 8);
static_assert(sizeof(wchar_t) == 2);
static_assert(std::endian::native == std::endian::little);

#define PLATFORM_DESKTOP (UNREALPROSPERO_TARGET_WINDOWS || UNREALPROSPERO_TARGET_LINUX)
#define PLATFORM_LITTLE_ENDIAN 1
#define PLATFORM_EXCEPTIONS_DISABLED 0
#define PLATFORM_SEH_EXCEPTIONS_DISABLED 1
#define PLATFORM_SUPPORTS_UNALIGNED_LOADS 1
#define PLATFORM_ENABLE_VECTORINTRINSICS 1
#define PLATFORM_COMPILER_DISTINGUISHES_INT_AND_LONG (!UNREALPROSPERO_TARGET_WINDOWS)
#define PLATFORM_WCHAR_IS_4_BYTES 0
#define PLATFORM_TCHAR_IS_CHAR16 0
#define PLATFORM_IS_ANSI_MALLOC_THREADSAFE 1
#define PLATFORM_USE_PTHREADS 0
#define PLATFORM_USE_GENERIC_LAUNCH_IMPLEMENTATION 1
#define PLATFORM_HAS_BSD_TIME 0
#define PLATFORM_ALWAYS_HAS_AESNI UNREALPROSPERO_TARGET_PS5
#define PLATFORM_CAN_SUPPORT_EDITORONLY_DATA 0
#define FORCEINLINE inline
#define PLATFORM_BREAK() UnrealProspero::Kernel::DebugBreak()
#define GCC_ALIGN(alignment) __attribute__((aligned(alignment)))
#define OPERATOR_DELETE_THROW_SPEC noexcept
#define OPERATOR_NEW_NOTHROW_SPEC noexcept
#define OPERATOR_DELETE_NOTHROW_SPEC noexcept

#endif
