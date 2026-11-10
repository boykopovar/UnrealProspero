#include "HAL/PlatformAtomics.h"
#include "prx/libkernel/include/Api.hpp"
#include <array>

namespace
{
    template<typename TValue>
    bool checkInteger()
    {
        TValue value = 4;
        if (FPlatformAtomics::InterlockedIncrement(&value) != 5) return false;
        if (FPlatformAtomics::InterlockedDecrement(&value) != 4) return false;
        if (FPlatformAtomics::InterlockedAdd(&value, TValue{3}) != 4 || value != 7) return false;
        if (FPlatformAtomics::InterlockedExchange(&value, TValue{10}) != 7) return false;
        if (FPlatformAtomics::InterlockedCompareExchange(&value, TValue{12}, TValue{9}) != 10 || value != 10) return false;
        if (FPlatformAtomics::InterlockedCompareExchange(&value, TValue{12}, TValue{10}) != 10 || value != 12) return false;
        if (FPlatformAtomics::InterlockedAnd(&value, TValue{10}) != 12 || value != 8) return false;
        if (FPlatformAtomics::InterlockedOr(&value, TValue{3}) != 8 || value != 11) return false;
        if (FPlatformAtomics::InterlockedXor(&value, TValue{7}) != 11 || value != 12) return false;
        FPlatformAtomics::AtomicStore(&value, TValue{21});
        if (FPlatformAtomics::AtomicRead(&value) != 21) return false;
        FPlatformAtomics::AtomicStore_Relaxed(&value, TValue{25});
        return FPlatformAtomics::AtomicRead_Relaxed(&value) == 25;
    }

    struct Counter
    {
        int64 value = 0;
    };

    void* APS5_VABI increment(void* argument)
    {
        auto& counter = *static_cast<Counter*>(argument);
        for (unsigned index = 0; index < 10000; ++index) FPlatformAtomics::InterlockedIncrement(&counter.value);
        return nullptr;
    }
}

bool CheckAtomics()
{
    if (!checkInteger<int8>() || !checkInteger<int16>() || !checkInteger<int32>() || !checkInteger<int64>()) return false;
    int first = 1;
    int second = 2;
    void* pointer = &first;
    if (FPlatformAtomics::InterlockedExchangePtr(&pointer, &second) != &first || pointer != &second) return false;
    if (FPlatformAtomics::InterlockedCompareExchangePointer(&pointer, &first, nullptr) != &second || pointer != &second) return false;
    if (FPlatformAtomics::InterlockedCompareExchangePointer(&pointer, &first, &second) != &second || pointer != &first) return false;
    Counter counter;
    std::array<Pthread, 4> threads{};
    for (Pthread& thread : threads)
    {
        if (scePthreadCreate(&thread, nullptr, increment, &counter, "AnyUnrealAtomic") != 0) return false;
    }
    for (Pthread thread : threads)
    {
        if (scePthreadJoin(thread, nullptr) != 0) return false;
    }
    return FPlatformAtomics::AtomicRead(&counter.value) == 40000;
}
