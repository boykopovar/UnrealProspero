#include "UnrealProspero/RHI/Query/OcclusionSnapshot.h"
#include "UnrealProspero/RHI/Command/MemoryAcquire.h"
#include "prx/libSceAgc/DcbFlow/include/Sync.hpp"
#include "prx/libSceAgc/DcbState/include/Registers.hpp"
#include <atomic>
#include <limits>
#include <memory>
#include <stdexcept>

namespace UnrealProspero::RHI
{
    OcclusionSnapshot::OcclusionSnapshot(std::uint32_t addressHigh) : counters(16 * 2 * sizeof(std::uint64_t), addressHigh), completion(addressHigh)
    {
        auto* values = static_cast<std::uint64_t*>(counters.Data());
        for (std::size_t index = 0; index < 32; ++index) std::construct_at(values + index, 0);
    }

    void OcclusionSnapshot::Begin(CommandBuffer& commands)
    {
        if (begun) throw std::runtime_error("Occlusion snapshot has already begun");
        begun = true;
        const GpuMemoryRange ranges[]{{counters.Data(), counters.Size()}};
        WriteMemoryAcquire(commands, ranges);
        sceAgcDcbSetCxRegisterDirect(&commands, {1, 0x11000106});
        sceAgcDcbEventWrite(&commands, 0x39, counters.Data());
    }

    void OcclusionSnapshot::End(CommandBuffer& commands)
    {
        if (!begun || ended) throw std::runtime_error("Occlusion snapshot is not active");
        ended = true;
        sceAgcDcbEventWrite(&commands, 0x39, static_cast<std::uint64_t*>(counters.Data()) + 1);
        completion.Write(commands);
    }

    bool OcclusionSnapshot::IsEnded() const { return ended; }

    bool OcclusionSnapshot::TryGetResult(std::uint64_t& result, bool wait) const
    {
        if (!ended) return false;
        if (wait) completion.Wait(10000000);
        else if (!completion.IsComplete()) return false;
        constexpr auto ready = std::uint64_t{1} << 63;
        std::uint64_t total = 0;
        bool available = false;
        auto* values = static_cast<std::uint64_t*>(counters.Data());
        for (std::size_t index = 0; index < 16; ++index)
        {
            const auto begin = std::atomic_ref(values[index * 2]).load(std::memory_order_acquire);
            const auto end = std::atomic_ref(values[index * 2 + 1]).load(std::memory_order_acquire);
            if (begin == 0 && end == 0) continue;
            if ((begin & end & ready) == 0) throw std::runtime_error("GPU completed an incomplete occlusion counter pair");
            available = true;
            const auto difference = (end - begin) & (ready - 1);
            if (difference > std::numeric_limits<std::uint64_t>::max() - total) throw std::overflow_error("Occlusion result exceeds 64 bits");
            total += difference;
        }
        if (!available) throw std::runtime_error("GPU completed without writing occlusion counters");
        result = total;
        return true;
    }
}
