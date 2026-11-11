#include "UnrealProspero/Kernel/ThreadAttributes.h"
#include "UnrealProspero/Kernel/Affinity.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"

namespace UnrealProspero::Kernel
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

    void ThreadAttributes::SetPriority(int priority)
    {
        const KernelSchedParam parameters{priority};
        CheckResult(scePthreadAttrSetschedparam(&handle, &parameters), "scePthreadAttrSetschedparam");
        CheckResult(scePthreadAttrSetinheritsched(&handle, 0), "scePthreadAttrSetinheritsched");
    }

    void ThreadAttributes::SetExclusiveScheduling(bool exclusive)
    {
        CheckResult(scePthreadAttrSetsolosched(&handle, exclusive ? 16 : 0), "scePthreadAttrSetsolosched");
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
