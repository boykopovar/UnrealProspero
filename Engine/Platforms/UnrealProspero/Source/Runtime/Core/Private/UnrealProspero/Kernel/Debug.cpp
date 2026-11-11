#include "UnrealProspero/Kernel/Debug.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"

namespace UnrealProspero::Kernel
{
    void DebugBreak()
    {
        CheckResult(raise_nid_postfix(5), "raise(SIGTRAP)");
    }
}
