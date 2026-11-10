#include "AnyUnreal/Kernel/ThreadAttributes.h"
#include "AnyUnreal/Kernel/Affinity.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"

namespace AnyUnreal::Kernel
{
    ThreadAttributes::ThreadAttributes()
    {
        CheckResult(scePthreadAttrInit(&handle), "scePthreadAttrInit");
    }

    ThreadAttributes::~ThreadAttributes() noexcept(false)
    {
        CheckResult(scePthreadAttrDestroy(&handle), "scePthreadAttrDestroy");
    }

    void ThreadAttributes::SetStackSize(std::size_t size)
    {
        CheckResult(scePthreadAttrSetstacksize(&handle, size), "scePthreadAttrSetstacksize");
    }

    void ThreadAttributes::SetAffinity(KernelCpumask mask)
    {
        if (mask == 0 || (mask & ~Affinity::Available()) != 0) throw std::invalid_argument("Thread affinity contains unavailable processors");
        CheckResult(scePthreadAttrSetaffinity(&handle, mask), "scePthreadAttrSetaffinity");
    }

    const PthreadAttr* ThreadAttributes::Get() const noexcept
    {
        return &handle;
    }
}
