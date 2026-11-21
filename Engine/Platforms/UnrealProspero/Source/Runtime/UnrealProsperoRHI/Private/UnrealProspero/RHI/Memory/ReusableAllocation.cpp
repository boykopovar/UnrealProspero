#include "UnrealProspero/RHI/Memory/ReusableAllocation.h"
#if UNREALPROSPERO_WITH_UE_TRACKING
#include "ProfilingDebugging/CsvProfiler.h"
#endif
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

        struct CachedAllocation
        {
            std::uint64_t ReleasedAt;
            std::unique_ptr<GpuMemoryBlock> Block;
        };

        struct AllocationCache
        {
            static constexpr std::size_t Capacity = 64 * 1024 * 1024;
            std::mutex Mutex;
            std::map<AllocationKey, std::vector<CachedAllocation>> Blocks;
            std::map<std::uint32_t, std::shared_ptr<GpuAddressSpace>> AddressSpaces;
            std::size_t Bytes = 0;
            std::uint64_t Serial = 0;
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
#if UNREALPROSPERO_WITH_UE_TRACKING
        CSV_CUSTOM_STAT_GLOBAL(UnrealProsperoGpuAllocationRequests, 1, ECsvCustomStatOp::Accumulate);
#endif
        auto& cache = getAllocationCache();
        {
            const std::lock_guard lock(cache.Mutex);
            const auto found = cache.Blocks.find({size, addressHigh, alignment});
            if (found != cache.Blocks.end())
            {
                if (found->second.empty()) throw std::logic_error("GPU allocation cache contains an empty bucket");
                allocation = std::move(found->second.back().Block);
                found->second.pop_back();
                cache.Bytes -= size;
                if (found->second.empty()) cache.Blocks.erase(found);
            }
        }
        if (allocation)
        {
#if UNREALPROSPERO_WITH_UE_TRACKING
            CSV_CUSTOM_STAT_GLOBAL(UnrealProsperoGpuAllocationCacheHits, 1, ECsvCustomStatOp::Accumulate);
#endif
            std::memset(allocation->Data(), 0, size);
        }
        else
        {
            std::shared_ptr<GpuAddressSpace> addressSpace;
            {
                const std::lock_guard lock(cache.Mutex);
                auto found = cache.AddressSpaces.find(addressHigh);
                if (found == cache.AddressSpaces.end()) found = cache.AddressSpaces.emplace(addressHigh, std::make_shared<GpuAddressSpace>(addressHigh)).first;
                addressSpace = found->second;
            }
            allocation = std::make_unique<GpuMemoryBlock>(std::move(addressSpace), size, alignment);
#if UNREALPROSPERO_WITH_UE_TRACKING
            CSV_CUSTOM_STAT_GLOBAL(UnrealProsperoGpuAllocationMappings, 1, ECsvCustomStatOp::Accumulate);
#endif
            const auto address = reinterpret_cast<std::uintptr_t>(allocation->Data());
            if ((address >> 32) != addressHigh || ((address + size - 1) >> 32) != addressHigh) throw std::runtime_error("Kernel mapped GPU buffer outside the shader address aperture");
        }
    }

    ReusableAllocation::~ReusableAllocation() noexcept(false)
    {
        auto& cache = getAllocationCache();
        const auto size = allocation->Size();
        if (size > AllocationCache::Capacity) return;
        const std::lock_guard lock(cache.Mutex);
        if (cache.Serial == std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("GPU allocation cache age exhausted");
        while (size > AllocationCache::Capacity - cache.Bytes)
        {
            auto oldest = cache.Blocks.end();
            for (auto it = cache.Blocks.begin(); it != cache.Blocks.end(); ++it)
            {
                if (it->second.empty()) throw std::logic_error("GPU allocation cache contains an empty bucket");
                if (oldest == cache.Blocks.end() || it->second.front().ReleasedAt < oldest->second.front().ReleasedAt) oldest = it;
            }
            if (oldest == cache.Blocks.end()) throw std::logic_error("GPU allocation cache byte accounting is inconsistent");
#if UNREALPROSPERO_WITH_UE_TRACKING
            CSV_CUSTOM_STAT_GLOBAL(UnrealProsperoGpuAllocationCacheEvictions, 1, ECsvCustomStatOp::Accumulate);
#endif
            const auto retiredSize = oldest->second.front().Block->Size();
            oldest->second.erase(oldest->second.begin());
            cache.Bytes -= retiredSize;
            if (oldest->second.empty()) cache.Blocks.erase(oldest);
        }
        cache.Blocks[{size, addressHigh, alignment}].push_back({++cache.Serial, std::move(allocation)});
        cache.Bytes += size;
    }

    void* ReusableAllocation::Data() const { return allocation->Data(); }
    std::size_t ReusableAllocation::Size() const { return allocation->Size(); }
}
