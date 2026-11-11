#include "AnyUnreal/RHI/Command/Submission.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libSceAgc/Cb/include/Sync.hpp"
#include "prx/libSceAgcDriver/Submit/include/Dcb.hpp"
#include "prx/libkernel/Time/include/Time.hpp"
#include <atomic>
#include <limits>
#include <stdexcept>

namespace AnyUnreal::RHI
{
    void SubmitAndWait(CommandBuffer& commands, std::uint64_t& completion, std::uint64_t value, std::uint64_t timeoutMicroseconds)
    {
        if (timeoutMicroseconds == 0 || reinterpret_cast<std::uintptr_t>(&completion) % std::atomic_ref<std::uint64_t>::required_alignment != 0) throw std::invalid_argument("Invalid AGC completion wait");
        std::atomic_ref fence(completion);
        if (fence.load(std::memory_order_acquire) == value) throw std::invalid_argument("AGC completion value has already been reached");
        const auto bottom = reinterpret_cast<std::uintptr_t>(commands.bottom);
        const auto top = reinterpret_cast<std::uintptr_t>(commands.top);
        const auto up = reinterpret_cast<std::uintptr_t>(commands.cursor_up);
        const auto down = reinterpret_cast<std::uintptr_t>(commands.cursor_down);
        if (bottom == 0 || bottom > up || up > down || down > top || ((bottom | top | up | down) & 3) != 0 || (top - bottom) / 4 > std::numeric_limits<std::uint32_t>::max()) throw std::invalid_argument("Invalid AGC submission command range");
        sceAgcCbReleaseMem(&commands, 0x2d, 0x70f, 0, 0, reinterpret_cast<Label*>(&completion), 2, value, 0, 0, 3, 0);
        const Packet packet{commands.bottom, static_cast<std::uint32_t>(commands.cursor_up - commands.bottom), 0, {}};
        std::atomic_thread_fence(std::memory_order_seq_cst);
        Kernel::CheckResult(sceAgcDriverSubmitDcb(&packet), "sceAgcDriverSubmitDcb");
        const auto start = sceKernelGetProcessTime();
        while (fence.load(std::memory_order_acquire) != value)
        {
            if (sceKernelGetProcessTime() - start >= timeoutMicroseconds) throw std::runtime_error("AGC submission completion timed out");
            Kernel::CheckResult(sceKernelUsleep_nid_postfix(1000), "sceKernelUsleep");
        }
    }
}
