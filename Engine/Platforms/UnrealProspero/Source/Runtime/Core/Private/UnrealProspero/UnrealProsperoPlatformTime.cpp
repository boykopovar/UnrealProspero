#include "UnrealProspero/Kernel/SystemConfiguration.h"
#include "UnrealProspero/UnrealProsperoPlatformTime.h"
#include "UnrealProspero/Kernel/Calendar.h"
#include "UnrealProspero/Kernel/Result.h"
#include "UnrealProspero/Mutex/ScopedLock.h"
#include "UnrealProspero/Memory/SystemAllocator.h"
#include <memory>
#include "prx/libkernel/include/Api.hpp"

namespace
{
    void assignTime(const UnrealProspero::Kernel::CalendarTime& time, int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond)
    {
        year = time.Year;
        month = time.Month;
        dayOfWeek = time.DayOfWeek;
        day = time.Day;
        hour = time.Hour;
        minute = time.Minute;
        second = time.Second;
        millisecond = time.Millisecond;
    }
}

void FUnrealProsperoPlatformTime::SystemTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond)
{
    assignTime(UnrealProspero::Kernel::Calendar::Local(UnrealProspero::Kernel::Clock::UnixTime()), year, month, dayOfWeek, day, hour, minute, second, millisecond);
}

void FUnrealProsperoPlatformTime::UtcTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond)
{
    assignTime(UnrealProspero::Kernel::Calendar::Utc(UnrealProspero::Kernel::Clock::UnixTime()), year, month, dayOfWeek, day, hour, minute, second, millisecond);
}

namespace
{
    int processorCount()
    {
        static const int count = UnrealProspero::Kernel::SystemConfiguration::LogicalProcessorCount();
        return count;
    }

    class CpuSample
    {
    public:
        void Update(bool process, uint64 minimumInterval = 0)
        {
            using UnrealProspero::Kernel::Clock;
            const uint64 wall = Clock::MonotonicNanoseconds();
            if (initialized && wall < wallTime) throw std::runtime_error("Monotonic clock moved backwards");
            if (initialized && wall - wallTime < minimumInterval) return;
            const uint64 cpu = process ? Clock::ProcessNanoseconds() : Clock::ThreadNanoseconds();
            if (initialized && cpu < cpuTime) throw std::runtime_error("CPU clock moved backwards");
            const int processors = processorCount();
            if (initialized && wall == wallTime) return;
            if (initialized)
            {
                const double relative = 100.0 * static_cast<double>(cpu - cpuTime) / static_cast<double>(wall - wallTime);
                usage = FCPUTime(static_cast<float>(relative / processors), static_cast<float>(relative));
                interval = static_cast<double>(cpu - cpuTime) / Clock::Frequency;
            }
            wallTime = wall;
            cpuTime = cpu;
            initialized = true;
        }

        FCPUTime Usage() const { return usage; }
        double Interval() const { return interval; }

    private:
        bool initialized = false;
        uint64 wallTime = 0;
        uint64 cpuTime = 0;
        FCPUTime usage{0, 0};
        double interval = 0;
    };

    struct ProcessSample
    {
        UE::FUnrealProsperoRecursiveMutex Mutex;
        CpuSample Sample;
    };

    ProcessSample& processSample()
    {
        static ProcessSample sample;
        return sample;
    }

    class ThreadSamples
    {
    public:
        ThreadSamples()
        {
            UnrealProspero::Kernel::CheckResult(scePthreadKeyCreate(&key, reinterpret_cast<pthread_key_destructor_func_t>(destroy)), "scePthreadKeyCreate");
        }

        ~ThreadSamples() noexcept(false)
        {
            void* sample = scePthreadGetspecific(key);
            UnrealProspero::Kernel::CheckResult(scePthreadSetspecific(key, nullptr), "scePthreadSetspecific");
            destroy(sample);
            UnrealProspero::Kernel::CheckResult(scePthreadKeyDelete(key), "scePthreadKeyDelete");
        }

        ThreadSamples(const ThreadSamples&) = delete;
        ThreadSamples& operator=(const ThreadSamples&) = delete;

        CpuSample& Get()
        {
            auto* sample = static_cast<CpuSample*>(scePthreadGetspecific(key));
            if (sample != nullptr) return *sample;
            sample = UnrealProspero::Memory::SystemAllocator<CpuSample>{}.allocate(1);
            std::construct_at(sample);
            const int result = scePthreadSetspecific(key, sample);
            if (result != 0)
            {
                destroy(sample);
                UnrealProspero::Kernel::CheckResult(result, "scePthreadSetspecific");
            }
            return *sample;
        }

    private:
        PthreadKey key{};

        static void APS5_VABI destroy(void* value)
        {
            if (value == nullptr) return;
            auto* sample = static_cast<CpuSample*>(value);
            std::destroy_at(sample);
            UnrealProspero::Memory::SystemAllocator<CpuSample>{}.deallocate(sample, 1);
        }
    };

    CpuSample& threadSample()
    {
        static ThreadSamples samples;
        return samples.Get();
    }
}

bool FUnrealProsperoPlatformTime::UpdateCPUTime(float)
{
    auto& state = processSample();
    UnrealProspero::ScopedLock lock(state.Mutex);
    state.Sample.Update(true);
    return true;
}

bool FUnrealProsperoPlatformTime::UpdateThreadCPUTime(float)
{
    threadSample().Update(false);
    return true;
}

FCPUTime FUnrealProsperoPlatformTime::GetCPUTime()
{
    auto& state = processSample();
    UnrealProspero::ScopedLock lock(state.Mutex);
    state.Sample.Update(true, UnrealProspero::Kernel::Clock::Frequency / 4);
    return state.Sample.Usage();
}

FCPUTime FUnrealProsperoPlatformTime::GetThreadCPUTime()
{
    return threadSample().Usage();
}

double FUnrealProsperoPlatformTime::GetLastIntervalCPUTimeInSeconds()
{
    auto& state = processSample();
    UnrealProspero::ScopedLock lock(state.Mutex);
    return state.Sample.Interval();
}

double FUnrealProsperoPlatformTime::GetLastIntervalThreadCPUTimeInSeconds()
{
    return threadSample().Interval();
}
