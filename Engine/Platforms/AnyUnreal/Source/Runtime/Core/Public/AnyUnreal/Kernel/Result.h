#ifndef ANYUNREAL_KERNEL_RESULT_H
#define ANYUNREAL_KERNEL_RESULT_H

#include "prx/libkernel/KernelErrors.hpp"
#include <stdexcept>
#include <string>

namespace AnyUnreal::Kernel
{
    inline void CheckResult(int result, const char* operation)
    {
        if (result != 0) throw std::runtime_error(std::string(operation) + ": " + std::to_string(result));
    }

    inline bool CheckLockResult(int result, const char* operation)
    {
        if (result == SCE_KERNEL_ERROR_EBUSY) return false;
        CheckResult(result, operation);
        return true;
    }
}

#endif
