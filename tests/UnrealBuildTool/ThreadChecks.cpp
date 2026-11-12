#include "UnrealProspero/Runnable/Priority.h"
#include "UnrealProspero/Kernel/KernelThread.h"
#include "UnrealProspero/Kernel/Affinity.h"
#include "UnrealProspero/Kernel/Event.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include <bit>
#include <stdexcept>

bool CheckThreads()
{
    using namespace UnrealProspero::Kernel;
    ThreadAttributes attributes;
    constexpr EThreadPriority priorities[]{TPri_TimeCritical, TPri_Highest, TPri_AboveNormal, TPri_Normal, TPri_SlightlyBelowNormal, TPri_BelowNormal, TPri_Lowest};
    int previous = 255;
    for (const auto priority : priorities)
    {
        const int mapped = UnrealProspero::Runnable::GetKernelPriority(priority);
        if (mapped <= previous || mapped > 767) return false;
        attributes.SetPriority(mapped);
        KernelSchedParam actual{};
        CheckResult(scePthreadAttrGetschedparam(attributes.Get(), &actual), "scePthreadAttrGetschedparam");
        if (actual.sched_priority != mapped) return false;
        previous = mapped;
    }
    try
    {
        UnrealProspero::Runnable::GetKernelPriority(TPri_Num);
        return false;
    }
    catch (const std::invalid_argument&)
    {
    }
    constexpr std::size_t stackSize = 2 * 1024 * 1024;
    const auto affinity = KernelCpumask{1} << std::countr_zero(Affinity::Available());
    attributes.SetStackSize(stackSize);
    attributes.SetPriority(600);
    attributes.SetExclusiveScheduling(true);
    int solo = 0;
    CheckResult(scePthreadAttrGetsolosched(attributes.Get(), &solo), "scePthreadAttrGetsolosched");
    if (solo != 16) return false;
    attributes.SetExclusiveScheduling(false);
    CheckResult(scePthreadAttrGetsolosched(attributes.Get(), &solo), "scePthreadAttrGetsolosched");
    if (solo != 0) return false;
    attributes.SetAffinity(affinity);
    const auto parent = scePthreadGetthreadid();
    bool passed = false;
    Event entered(true);
    Event release(true);
    Thread thread([&]
    {
        PthreadAttr actual = nullptr;
        CheckResult(scePthreadAttrInit(&actual), "scePthreadAttrInit");
        CheckResult(scePthreadAttrGet(scePthreadSelf(), &actual), "scePthreadAttrGet");
        std::size_t actualStack = 0;
        CheckResult(scePthreadAttrGetstacksize(&actual, &actualStack), "scePthreadAttrGetstacksize");
        KernelSchedParam priority{};
        int inherit = -1;
        CheckResult(scePthreadAttrGetschedparam(&actual, &priority), "scePthreadAttrGetschedparam");
        CheckResult(scePthreadAttrGetinheritsched(&actual, &inherit), "scePthreadAttrGetinheritsched");
        CheckResult(scePthreadAttrDestroy(&actual), "scePthreadAttrDestroy");
        passed = priority.sched_priority == 600 && inherit == 0 && actualStack == stackSize && Affinity::Get(scePthreadSelf()) == affinity && scePthreadGetthreadid() != parent;
        entered.Trigger();
        release.Wait();
    }, "UnrealProsperoThread", &attributes);
    const bool started = entered.Wait(5000);
    const bool valid = started && thread.IsJoinable() && thread.Get() != scePthreadSelf();
    release.Trigger();
    thread.Join();
    if (!valid || !passed || thread.IsJoinable()) return false;
    thread.Join();
    try
    {
        thread.Get();
        return false;
    }
    catch (const std::logic_error&)
    {
    }
    int value = 0;
    {
        Thread scoped([&] { value = 42; }, "UnrealProsperoScoped");
    }
    if (value != 42) return false;
    Thread throwing([] { throw std::range_error("thread failure"); }, "UnrealProsperoThrow");
    try
    {
        throwing.Join();
        return false;
    }
    catch (const std::range_error& error)
    {
        if (std::string_view(error.what()) != "thread failure") return false;
    }
    if (throwing.IsJoinable()) return false;
    throwing.Join();
    try
    {
        Thread empty({}, "Empty");
        return false;
    }
    catch (const std::invalid_argument&)
    {
    }
    try
    {
        Thread invalid([] {}, std::string_view("a\0b", 3));
        return false;
    }
    catch (const std::invalid_argument&)
    {
    }
    try
    {
        attributes.SetAffinity(0);
        return false;
    }
    catch (const std::invalid_argument&)
    {
    }
    try
    {
        attributes.SetStackSize(1);
        return false;
    }
    catch (const Error&)
    {
    }
    try
    {
        attributes.SetPriority(0);
        return false;
    }
    catch (const Error&)
    {
    }
    return true;
}
