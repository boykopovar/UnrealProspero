#ifndef ANYUNREAL_MUTEX_SCOPED_LOCK_H
#define ANYUNREAL_MUTEX_SCOPED_LOCK_H

#include "AnyUnreal/Mutex/RecursiveMutex.h"

namespace AnyUnreal
{
    class ScopedLock
    {
    public:
        explicit ScopedLock(UE::FAnyUnrealRecursiveMutex& mutex) : mutex(mutex)
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
        UE::FAnyUnrealRecursiveMutex& mutex;
    };
}

#endif
