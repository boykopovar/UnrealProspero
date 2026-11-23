#include "prx/libSceRandom/include/Api.hpp"
#include <cerrno>
#include <cstddef>
#include <limits>

extern "C" std::ptrdiff_t UnrealProsperoOpenSslRandom(void* buffer, std::size_t size)
{
    if (size > static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max()))
    {
        errno = EINVAL;
        return -1;
    }
    auto* bytes = static_cast<std::byte*>(buffer);
    constexpr std::size_t maxRequestSize = 64;
    for (std::size_t offset = 0; offset < size;)
    {
        const std::size_t left = size - offset;
        const std::size_t count = left < maxRequestSize ? left : maxRequestSize;
        if (sceRandomGetRandomNumber(bytes + offset, count) != 0)
        {
            errno = EIO;
            return -1;
        }
        offset += count;
    }
    return static_cast<std::ptrdiff_t>(size);
}
