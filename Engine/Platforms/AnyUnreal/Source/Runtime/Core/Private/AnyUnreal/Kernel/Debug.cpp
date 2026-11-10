#include "AnyUnreal/Kernel/Debug.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"

namespace AnyUnreal::Kernel
{
    void DebugBreak()
    {
        CheckResult(raise_nid_postfix(5), "raise(SIGTRAP)");
    }
}
