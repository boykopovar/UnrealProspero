#include "UnrealProspero/Text/SecureString.h"
#if !UNREALPROSPERO_TARGET_WINDOWS
#include "prx/libc/include/Api.hpp"
extern "C" std::size_t strnlen_s(const char* text, std::size_t limit)
{
    return strnlen_s_nid_postfix(text, limit);
}

extern "C" std::size_t wcsnlen_s(const wchar_t* text, std::size_t limit)
{
    if (!text) return 0;
    std::size_t length = 0;
    while (length < limit && text[length] != 0) ++length;
    return length;
}
#endif
