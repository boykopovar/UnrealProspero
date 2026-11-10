#include "AnyUnreal/Kernel/Thread.h"
#include "AnyUnreal/Kernel/Affinity.h"
#include "AnyUnreal/Kernel/Event.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include <bit>
#include <stdexcept>

bool CheckThreads()
{
    using namespace AnyUnreal::Kernel;
    ThreadAttributes attributes;
    constexpr std::size_t stackSize = 2 * 1024 * 1024;
    const auto affinity = KernelCpumask{1} << std::countr_zero(Affinity::Available());
    attributes.SetStackSize(stackSize);
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
        CheckResult(scePthreadAttrDestroy(&actual), "scePthreadAttrDestroy");
        passed = actualStack == stackSize && Affinity::Get(scePthreadSelf()) == affinity && scePthreadGetthreadid() != parent;
        entered.Trigger();
        release.Wait();
    }, "AnyUnrealThread", &attributes);
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
        Thread scoped([&] { value = 42; }, "AnyUnrealScoped");
    }
    if (value != 42) return false;
    Thread throwing([] { throw std::range_error("thread failure"); }, "AnyUnrealThrow");
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
    return true;
}
