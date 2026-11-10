#ifndef ANYUNREAL_PLATFORM_H
#define ANYUNREAL_PLATFORM_H

#include "Clang/ClangPlatform.h"
#include "GenericPlatform/GenericPlatform.h"
#include "AnyUnreal/Kernel/Debug.h"
#include <bit>
#include <cstddef>

#if ANYUNREAL_TARGET_WINDOWS + ANYUNREAL_TARGET_LINUX + ANYUNREAL_TARGET_PS5 != 1
#error Exactly one AnyUnreal target must be selected
#endif

struct FAnyUnrealPlatformTypes : FGenericPlatformTypes
{
    using DWORD = unsigned int;
    using SIZE_T = std::size_t;
    using SSIZE_T = std::ptrdiff_t;
    using TYPE_OF_NULL = decltype(NULL);
};

using FPlatformTypes = FAnyUnrealPlatformTypes;

static_assert(sizeof(void*) == 8);
static_assert(sizeof(wchar_t) == 2);
static_assert(std::endian::native == std::endian::little);

#define PLATFORM_DESKTOP (ANYUNREAL_TARGET_WINDOWS || ANYUNREAL_TARGET_LINUX)
#define PLATFORM_LITTLE_ENDIAN 1
#define PLATFORM_EXCEPTIONS_DISABLED 0
#define PLATFORM_SEH_EXCEPTIONS_DISABLED 1
#define PLATFORM_SUPPORTS_UNALIGNED_LOADS 1
#define PLATFORM_COMPILER_DISTINGUISHES_INT_AND_LONG (!ANYUNREAL_TARGET_WINDOWS)
#define PLATFORM_WCHAR_IS_4_BYTES 0
#define PLATFORM_TCHAR_IS_CHAR16 0
#define PLATFORM_IS_ANSI_MALLOC_THREADSAFE 1
#define PLATFORM_CAN_SUPPORT_EDITORONLY_DATA 0
#define FORCEINLINE inline
#define PLATFORM_BREAK() AnyUnreal::Kernel::DebugBreak()
#define GCC_ALIGN(alignment) __attribute__((aligned(alignment)))
#define OPERATOR_DELETE_THROW_SPEC noexcept
#define OPERATOR_NEW_NOTHROW_SPEC noexcept
#define OPERATOR_DELETE_NOTHROW_SPEC noexcept

#endif
