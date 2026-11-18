#include "prx/libkernel/include/Api.hpp"
#include <cstdio>
#include <cstdlib>

extern "C" [[noreturn]] void __assert_fail(const char* expression, const char* file, unsigned int line, const char* function) noexcept
{
    char message[1024];
    const int length = std::snprintf(message, sizeof(message), "%s:%u: %s: assertion `%s` failed\n", file, line, function, expression);
    if (length > 0)
    {
        constexpr int limit = static_cast<int>(sizeof(message) - 1);
        write_nid_postfix(2, message, length < limit ? length : limit);
    }
    std::abort();
}
