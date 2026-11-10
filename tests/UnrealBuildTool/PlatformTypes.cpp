#include "CoreTypes.h"
#include <cstddef>
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
