#ifndef UNREALPROSPERO_KERNEL_POSIX_RESULT_H
#define UNREALPROSPERO_KERNEL_POSIX_RESULT_H

#include "UnrealProspero/Kernel/Error.h"
#include <bit>
#include <cerrno>

namespace UnrealProspero::Kernel
{
    inline void CheckPosixResult(int result, const char* operation)
    {
        if (result == 0) return;
        const int error = errno;
        if (result != -1 || error <= 0 || error > 0xffff) throw std::runtime_error("Invalid POSIX error result");
        throw Error(std::bit_cast<std::int32_t>(0x80020000u | static_cast<std::uint32_t>(error)), operation);
    }
}

#endif
