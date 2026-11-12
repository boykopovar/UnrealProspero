#include "HAL/UESemaphore.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include <array>
#include <atomic>
#include <stdexcept>

namespace
{
    struct Waiters
    {
        FSemaphore semaphore{0, 2};
        std::atomic<unsigned> acquired{0};
    };

    void* APS5_VABI acquire(void* argument)
    {
        auto& waiters = *static_cast<Waiters*>(argument);
        waiters.semaphore.Acquire();
        waiters.acquired.fetch_add(1);
        return nullptr;
    }
}

bool CheckSemaphores()
{
    FSemaphore semaphore{1, 2};
    if (!semaphore.TryAcquire() || semaphore.TryAcquire()) return false;
    if (semaphore.TryAcquire(FTimespan(10000))) return false;
    semaphore.Release(2);
    if (!semaphore.TryAcquire() || !semaphore.TryAcquire() || semaphore.TryAcquire()) return false;
    bool rejected = false;
    try { semaphore.Release(0); }
    catch (const std::invalid_argument&) { rejected = true; }
    if (!rejected) return false;
    Waiters waiters;
    std::array<Pthread, 2> threads{};
    for (auto& thread : threads) UnrealProspero::Kernel::CheckResult(scePthreadCreate(&thread, nullptr, acquire, &waiters, "UnrealProsperoSemaCheck"), __func__);
    waiters.semaphore.Release(2);
    for (auto thread : threads) UnrealProspero::Kernel::CheckResult(scePthreadJoin(thread, nullptr), __func__);
    return waiters.acquired.load() == 2 && !waiters.semaphore.TryAcquire();
}
