#ifndef UNREALPROSPERO_SEMAPHORE_H
#define UNREALPROSPERO_SEMAPHORE_H

#include "CoreTypes.h"
#include "Misc/Timespan.h"
#include "SceTypes.hpp"

class FUnrealProsperoSemaphore final
{
public:
    CORE_API FUnrealProsperoSemaphore(int32 initialCount, int32 maxCount);
    CORE_API ~FUnrealProsperoSemaphore() noexcept(false);
    FUnrealProsperoSemaphore(const FUnrealProsperoSemaphore&) = delete;
    FUnrealProsperoSemaphore& operator=(const FUnrealProsperoSemaphore&) = delete;
    CORE_API void Acquire();
    CORE_API bool TryAcquire(FTimespan timeout = FTimespan::Zero());
    CORE_API void Release(int32 count = 1);

private:
    KernelSema handle = nullptr;
};

using FSemaphore = FUnrealProsperoSemaphore;

#endif
