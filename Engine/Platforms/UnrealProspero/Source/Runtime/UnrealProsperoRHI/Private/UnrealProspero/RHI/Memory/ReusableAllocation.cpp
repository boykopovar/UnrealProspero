#include "UnrealProspero/RHI/Memory/ReusableAllocation.h"
#include <algorithm>
#include <bit>
#include <cstring>
#include <limits>
#include <map>
#include <mutex>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace UnrealProspero::RHI
{
    namespace
    {
        using AllocationKey = std::tuple<std::size_t, std::uint32_t, std::size_t>;

        struct AllocationCache
        {
            static constexpr std::size_t Capacity = 64 * 1024 * 1024;
            std::mutex Mutex;
            std::map<AllocationKey, std::vector<std::unique_ptr<Kernel::DirectAllocation>>> Blocks;
            std::size_t Bytes = 0;
        };

        AllocationCache& getAllocationCache()
        {
            static AllocationCache cache;
            return cache;
        }
    }

    ReusableAllocation::ReusableAllocation(std::size_t size, std::uint32_t addressHigh, std::size_t alignment) : addressHigh(addressHigh), alignment(alignment)
    {
        if (addressHigh > 0x7fff) throw std::invalid_argument("GPU buffer aperture exceeds user address space");
        if (size == 0 || size % Kernel::DirectAllocation::PageSize != 0 || size > std::numeric_limits<std::uint32_t>::max()) throw std::invalid_argument("Invalid reusable GPU allocation size");
        if (alignment < Kernel::DirectAllocation::PageSize || !std::has_single_bit(alignment)) throw std::invalid_argument("Invalid reusable GPU allocation alignment");
        auto& cache = getAllocationCache();
        {
            const std::lock_guard lock(cache.Mutex);
            const auto found = cache.Blocks.find({size, addressHigh, alignment});
            if (found != cache.Blocks.end())
            {
                allocation = std::move(found->second.back());
                found->second.pop_back();
                cache.Bytes -= size;
                if (found->second.empty()) cache.Blocks.erase(found);
            }
        }
        if (allocation)
        {
            std::memset(allocation->Data(), 0, size);
        }
        else
        {
            const auto base = static_cast<std::uint64_t>(addressHigh) << 32;
            const auto hint = base == 0 ? std::max<std::uint64_t>(65536, alignment) : base;
            allocation = std::make_unique<Kernel::DirectAllocation>(size, alignment, Kernel::DirectMemoryAccess::CpuGpu, reinterpret_cast<void*>(static_cast<std::uintptr_t>(hint)));
            const auto address = reinterpret_cast<std::uintptr_t>(allocation->Data());
            if ((address >> 32) != addressHigh || ((address + size - 1) >> 32) != addressHigh) throw std::runtime_error("Kernel mapped GPU buffer outside the shader address aperture");
        }
    }

    ReusableAllocation::~ReusableAllocation() noexcept(false)
    {
        auto& cache = getAllocationCache();
        const auto size = allocation->Size();
        const std::lock_guard lock(cache.Mutex);
        if (size <= AllocationCache::Capacity - cache.Bytes)
        {
            cache.Blocks[{size, addressHigh, alignment}].push_back(std::move(allocation));
            cache.Bytes += size;
        }
    }

    void* ReusableAllocation::Data() const { return allocation->Data(); }
    std::size_t ReusableAllocation::Size() const { return allocation->Size(); }
}
