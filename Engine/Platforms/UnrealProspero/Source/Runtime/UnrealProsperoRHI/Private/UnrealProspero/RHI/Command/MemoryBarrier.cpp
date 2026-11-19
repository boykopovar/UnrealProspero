#include "UnrealProspero/RHI/Command/MemoryBarrier.h"
#include "prx/libSceAgc/Cb/include/Sync.hpp"
#include "prx/libSceAgc/DcbFlow/include/Sync.hpp"
#include <limits>
#include <memory>
#include <stdexcept>

namespace UnrealProspero::RHI
{
    MemoryBarrier::MemoryBarrier(std::uint32_t addressHigh) : completion(sizeof(std::uint64_t), addressHigh)
    {
        std::construct_at(static_cast<std::uint64_t*>(completion.Data()), 0);
    }

    void MemoryBarrier::Release(CommandBuffer& commands)
    {
        if (released) throw std::runtime_error("GPU memory barrier has already been released");
        released = true;
        sceAgcCbReleaseMem(&commands, 0x2d, 0x70f, 0, 0, static_cast<Label*>(completion.Data()), 2, 1, 0, 0, 3, 0);
    }

    void MemoryBarrier::Acquire(CommandBuffer& commands, std::span<const GpuMemoryRange> ranges)
    {
        if (!released || acquired) throw std::runtime_error("GPU memory barrier must be released once before acquisition");
        acquired = true;
        sceAgcDcbWaitRegMem(&commands, 1, 3, 0, 0, completion.Data(), 1, std::numeric_limits<std::uint64_t>::max(), 40);
        WriteMemoryAcquire(commands, ranges);
    }
}
