#include "AnyUnreal/Kernel/FileLock.h"
#include "AnyUnreal/Kernel/Clock.h"
#include "AnyUnreal/Kernel/PosixResult.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include "prx/libkernel/Time/include/Time.hpp"
#include <algorithm>

namespace AnyUnreal::Kernel
{
    FileLock::FileLock(std::string_view path, std::uint64_t timeoutNanoseconds) : descriptor(Descriptor::Open(path, 0x202, 0666))
    {
        const auto start = Clock::MonotonicNanoseconds();
        for (;;)
        {
            const int result = flock_nid_postfix(descriptor.Get(), 6);
            if (result == 0)
            {
                owned = true;
                return;
            }
            if (result != -1 || (errno != EWOULDBLOCK && errno != EINTR)) CheckPosixResult(result, "flock");
            const auto now = Clock::MonotonicNanoseconds();
            if (now < start) throw std::runtime_error("Monotonic clock moved backwards");
            const auto elapsed = now - start;
            if (elapsed >= timeoutNanoseconds) return;
            const auto remaining = timeoutNanoseconds - elapsed;
            const auto microseconds = std::min<std::uint64_t>(remaining / 1000 + (remaining % 1000 != 0), 1000);
            const int slept = sceKernelUsleep_nid_postfix(static_cast<unsigned>(microseconds));
            if (slept != SCE_KERNEL_ERROR_EINTR) CheckResult(slept, "sceKernelUsleep");
        }
    }

    FileLock::~FileLock() noexcept(false)
    {
        Release();
    }

    void FileLock::Release()
    {
        if (!owned) return;
        CheckPosixResult(flock_nid_postfix(descriptor.Get(), 8), "flock unlock");
        owned = false;
    }
}
