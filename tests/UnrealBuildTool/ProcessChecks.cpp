#include "HAL/PlatformProcess.h"
#include "HAL/PlatformAffinity.h"
#include "UnrealProspero/Kernel/Affinity.h"
#include "prx/libkernel/include/Api.hpp"
#include <bit>
#include "UnrealProspero/Kernel/Clock.h"
#include <limits>
#include <stdexcept>
#include <string_view>

namespace
{
    class AffinityRestore
    {
    public:
        AffinityRestore() : mask(UnrealProspero::Kernel::Affinity::Get(scePthreadSelf())) {}
        ~AffinityRestore() noexcept(false) { UnrealProspero::Kernel::Affinity::Set(scePthreadSelf(), mask); }

    private:
        KernelCpumask mask;
    };

    bool checkIdentity()
    {
        using UnrealProspero::Kernel::Affinity;
        const auto* base = FPlatformProcess::BaseDir();
        const std::basic_string_view<TCHAR> directory(base);
        if (directory.empty() || directory.back() != '/' || base != FPlatformProcess::BaseDir()) return false;
        if (FPlatformProcess::GetCurrentProcessId() != static_cast<uint32>(getpid_nid_postfix())) return false;
        AffinityRestore restore;
        const auto available = Affinity::Available();
        const auto selected = KernelCpumask{1} << std::countr_zero(available);
        FPlatformProcess::SetThreadAffinityMask(selected);
        if (Affinity::Get(scePthreadSelf()) != selected) return false;
        const auto core = FPlatformProcess::GetCurrentCoreNumber();
        if (core >= 64 || (selected & (KernelCpumask{1} << core)) == 0) return false;
        FPlatformProcess::SetThreadAffinityMask(FPlatformAffinity::GetNoAffinityMask());
        if (Affinity::Get(scePthreadSelf()) != available) return false;
        try
        {
            FPlatformProcess::SetThreadAffinityMask(0);
            return false;
        }
        catch (const std::invalid_argument&)
        {
        }
        if (available != std::numeric_limits<KernelCpumask>::max())
        {
            try
            {
                Affinity::Set(scePthreadSelf(), ~available);
                return false;
            }
            catch (const std::invalid_argument&)
            {
            }
        }
        return true;
    }

    template<typename TException>
    bool rejects(float seconds)
    {
        try
        {
            FPlatformProcess::SleepNoStats(seconds);
        }
        catch (const TException&)
        {
            return true;
        }
        return false;
    }
}

bool CheckProcess()
{
    if (!checkIdentity()) return false;
    if (!rejects<std::invalid_argument>(-1)) return false;
    if (!rejects<std::invalid_argument>(std::numeric_limits<float>::infinity())) return false;
    if (!rejects<std::invalid_argument>(std::numeric_limits<float>::quiet_NaN())) return false;
    if (!rejects<std::overflow_error>(std::numeric_limits<float>::max())) return false;
    FPlatformProcess::YieldThread();
    FPlatformProcess::SleepNoStats(0);
    using UnrealProspero::Kernel::Clock;
    const auto start = Clock::MonotonicNanoseconds();
    FPlatformProcess::SleepNoStats(0.02f);
    const auto end = Clock::MonotonicNanoseconds();
    if (end < start || end - start < 20000000) return false;
    FPlatformProcess::SleepNoStats(std::numeric_limits<float>::denorm_min());
    FProcHandle empty;
    if (empty.IsValid()) return false;
    FProcHandle handle(42);
    if (!handle.IsValid() || handle.Get() != 42) return false;
    handle.Reset();
    return !handle.IsValid();
}
