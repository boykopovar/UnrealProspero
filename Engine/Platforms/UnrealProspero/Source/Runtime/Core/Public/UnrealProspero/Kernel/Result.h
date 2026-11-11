#ifndef UNREALPROSPERO_KERNEL_RESULT_H
#define UNREALPROSPERO_KERNEL_RESULT_H

#include "prx/libkernel/KernelErrors.hpp"
#include "UnrealProspero/Kernel/Error.h"

namespace UnrealProspero::Kernel
{
    inline void CheckResult(int result, const char* operation)
    {
        if (result != 0) throw Error(result, operation);
    }

    inline bool CheckLockResult(int result, const char* operation)
    {
        if (result == SCE_KERNEL_ERROR_EBUSY) return false;
        CheckResult(result, operation);
        return true;
    }
}

#endif
