#include "UnrealProspero/RHI/Command/Completion.h"
#if UNREALPROSPERO_WITH_UE_TRACKING
#include "ProfilingDebugging/CsvProfiler.h"
#endif
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libSceAgc/Cb/include/Sync.hpp"
#include "prx/libkernel/Time/include/Time.hpp"
#include <atomic>
#include <memory>
#include <stdexcept>

namespace UnrealProspero::RHI
{
    Completion::Completion(std::uint32_t addressHigh) : storage(sizeof(std::uint64_t), addressHigh)
    {
        std::construct_at(static_cast<std::uint64_t*>(storage.Data()), 0);
    }

    void Completion::Write(CommandBuffer& commands)
    {
        if (written) throw std::runtime_error("GPU completion marker has already been recorded");
        written = true;
        sceAgcCbReleaseMem(&commands, 0x14, 0x70f, 0, 0, static_cast<Label*>(storage.Data()), 2, 1, 0, 0, 3, 0);
    }

    bool Completion::IsComplete() const
    {
        return written && std::atomic_ref(*static_cast<std::uint64_t*>(storage.Data())).load(std::memory_order_acquire) == 1;
    }

    void Completion::Wait(std::uint64_t timeoutMicroseconds) const
    {
#if UNREALPROSPERO_WITH_UE_TRACKING
        CSV_SCOPED_TIMING_STAT_EXCLUSIVE(UnrealProsperoWaitForGpuCompletion);
#endif
        if (!written || timeoutMicroseconds == 0) throw std::invalid_argument("GPU completion wait requires a recorded marker and a positive timeout");
        const auto start = sceKernelGetProcessTime();
        while (!IsComplete())
        {
            if (sceKernelGetProcessTime() - start >= timeoutMicroseconds) throw std::runtime_error("GPU completion timed out");
            Kernel::CheckResult(sceKernelUsleep_nid_postfix(1000), "sceKernelUsleep");
        }
    }
}
