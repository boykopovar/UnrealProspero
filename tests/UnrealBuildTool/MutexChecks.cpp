#include "HAL/CriticalSection.h"
#include "prx/libkernel/include/Api.hpp"
#include <array>

namespace
{
    struct Probe
    {
        FCriticalSection* recursive;
        UE::FPlatformSharedMutex* shared;
        bool recursiveAvailable = false;
        bool writerAvailable = false;
        bool readerAvailable = false;
    };

    void* APS5_VABI probeLocks(void* argument)
    {
        auto& probe = *static_cast<Probe*>(argument);
        probe.recursiveAvailable = probe.recursive->TryLock();
        if (probe.recursiveAvailable) probe.recursive->Unlock();
        probe.writerAvailable = probe.shared->TryLock();
        if (probe.writerAvailable) probe.shared->Unlock();
        probe.readerAvailable = probe.shared->TryLockShared();
        if (probe.readerAvailable) probe.shared->UnlockShared();
        return nullptr;
    }

    bool runProbe(Probe& probe, bool recursive, bool writer, bool reader)
    {
        Pthread thread;
        if (scePthreadCreate(&thread, nullptr, probeLocks, &probe, "AnyUnrealMutex") != 0) return false;
        if (scePthreadJoin(thread, nullptr) != 0) return false;
        return probe.recursiveAvailable == recursive && probe.writerAvailable == writer && probe.readerAvailable == reader;
    }

    struct Counter
    {
        FCriticalSection mutex;
        unsigned value = 0;
    };

    void* APS5_VABI incrementCounter(void* argument)
    {
        auto& counter = *static_cast<Counter*>(argument);
        for (unsigned index = 0; index < 2000; ++index)
        {
            counter.mutex.Lock();
            ++counter.value;
            counter.mutex.Unlock();
        }
        return nullptr;
    }
}

bool CheckMutexes()
{
    FCriticalSection recursive;
    UE::FPlatformSharedMutex shared;
    Probe probe{&recursive, &shared};
    recursive.Lock();
    if (!recursive.TryLock()) return false;
    shared.LockShared();
    if (!runProbe(probe, false, false, true)) return false;
    recursive.Unlock();
    if (!runProbe(probe, false, false, true)) return false;
    recursive.Unlock();
    shared.UnlockShared();
    shared.Lock();
    if (!runProbe(probe, true, false, false)) return false;
    shared.Unlock();
    if (!runProbe(probe, true, true, true)) return false;
    FRWLock rwLock;
    rwLock.ReadLock();
    if (rwLock.TryWriteLock()) return false;
    rwLock.ReadUnlock();
    if (!rwLock.TryWriteLock()) return false;
    rwLock.WriteUnlock();
    rwLock.WriteLock();
    if (rwLock.TryReadLock()) return false;
    rwLock.WriteUnlock();
    if (!rwLock.TryReadLock()) return false;
    rwLock.ReadUnlock();
    Counter counter;
    std::array<Pthread, 4> threads{};
    for (Pthread& thread : threads)
    {
        if (scePthreadCreate(&thread, nullptr, incrementCounter, &counter, "AnyUnrealCounter") != 0) return false;
    }
    for (Pthread thread : threads)
    {
        if (scePthreadJoin(thread, nullptr) != 0) return false;
    }
    return counter.value == 8000;
}
