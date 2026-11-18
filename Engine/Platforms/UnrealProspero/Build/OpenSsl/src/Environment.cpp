#include "prx/libc/include/Api.hpp"
#include "prx/libkernel/include/Api.hpp"

extern "C" char* secure_getenv(const char* name) noexcept
{
    if (issetugid_nid_postfix() != 0) return nullptr;
    return getenv_nid_postfix(name);
}

extern "C" int UnrealProsperoOpenSslToLower(int character)
{
    return tolower_nid_postfix(character);
}
