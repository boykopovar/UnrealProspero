#ifndef UNREALPROSPERO_MUTEX_SCOPED_LOCK_H
#define UNREALPROSPERO_MUTEX_SCOPED_LOCK_H

#include "UnrealProspero/Mutex/UnrealProsperoRecursiveMutex.h"

namespace UnrealProspero
{
    class ScopedLock
    {
    public:
        explicit ScopedLock(UE::FUnrealProsperoRecursiveMutex& mutex) : mutex(mutex)
        {
            mutex.Lock();
        }

        ~ScopedLock() noexcept(false)
        {
            mutex.Unlock();
        }

        ScopedLock(const ScopedLock&) = delete;
        ScopedLock& operator=(const ScopedLock&) = delete;

    private:
        UE::FUnrealProsperoRecursiveMutex& mutex;
    };
}

#endif
