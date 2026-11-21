#include "UnrealProspero/RHI/Memory/GpuAddressSpace.h"
#include "UnrealProspero/Kernel/DirectAllocation.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include <algorithm>
#include <bit>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <utility>

namespace UnrealProspero::RHI
{
    GpuAddressSpace::GpuAddressSpace(std::uint32_t addressHigh)
    {
        if (addressHigh > 0x7fff) throw std::invalid_argument("GPU address aperture exceeds user address space");
        const auto skipped = addressHigh == 0 ? std::size_t{65536} : 0;
        base = (static_cast<std::uintptr_t>(addressHigh) << 32) + skipped;
        size = (std::uint64_t{1} << 32) - skipped;
        const auto end = base + size;
        auto cursor = base;
        constexpr auto page = Kernel::MemoryPageSize;
        while (cursor < end)
        {
            VirtualQueryInfo info{};
            const auto result = sceKernelVirtualQuery(reinterpret_cast<void*>(cursor), 1, &info, sizeof(info));
            if (result == SCE_KERNEL_ERROR_EACCES)
            {
                reserveRange(cursor, end);
                break;
            }
            Kernel::CheckResult(result, "sceKernelVirtualQuery");
            if (info.start >= info.end || info.end <= cursor) throw std::runtime_error("Invalid virtual memory query extent");
            if (info.start > cursor)
            {
                const auto gapEnd = std::min(info.start, end) & ~(static_cast<std::uintptr_t>(page) - 1);
                if (gapEnd > cursor) reserveRange(cursor, gapEnd);
            }
            if (info.end >= end) break;
            cursor = (info.end + page - 1) & ~(static_cast<std::uintptr_t>(page) - 1);
        }
        if (freeRanges.empty()) throw std::runtime_error("GPU shader address aperture has no available virtual ranges");
    }

    void GpuAddressSpace::reserveRange(std::uintptr_t begin, std::uintptr_t end)
    {
        if (begin < base || begin >= end || end - base > size) throw std::logic_error("Invalid GPU address reservation extent");
        auto reservation = std::make_unique<GpuAddressReservation>(reinterpret_cast<void*>(begin), end - begin);
        if (!freeRanges.emplace(begin - base, end - begin).second) throw std::logic_error("GPU address reservations overlap");
        reservations.push_back(std::move(reservation));
    }

    GpuAddressSpace::~GpuAddressSpace() noexcept(false)
    {
        if (!allocations.empty()) throw std::logic_error("GPU address space still has live allocations");
    }

    void* GpuAddressSpace::Allocate(std::size_t bytes, std::size_t alignment)
    {
        if (bytes == 0 || bytes % Kernel::DirectAllocation::PageSize != 0 || bytes > size) throw std::invalid_argument("Invalid GPU address allocation size");
        if (alignment < Kernel::DirectAllocation::PageSize || !std::has_single_bit(alignment) || alignment > size) throw std::invalid_argument("Invalid GPU address allocation alignment");
        const std::lock_guard lock(mutex);
        for (auto it = freeRanges.begin(); it != freeRanges.end(); ++it)
        {
            const auto address = base + it->first;
            if (address > std::numeric_limits<std::uintptr_t>::max() - (alignment - 1)) throw std::overflow_error("GPU address alignment overflow");
            const auto aligned = (address + alignment - 1) & ~(static_cast<std::uintptr_t>(alignment) - 1);
            const auto prefix = aligned - address;
            if (prefix > it->second || bytes > it->second - prefix) continue;
            const auto offset = aligned - base;
            const auto tail = it->second - prefix - bytes;
            const auto inserted = allocations.emplace(offset, bytes);
            if (!inserted.second) throw std::logic_error("GPU address allocation overlaps a live range");
            try
            {
                if (prefix && tail && !freeRanges.emplace(offset + bytes, tail).second) throw std::logic_error("GPU free address ranges overlap");
                if (prefix) it->second = prefix;
                else if (tail)
                {
                    auto remainder = freeRanges.extract(it);
                    remainder.key() = offset + bytes;
                    remainder.mapped() = tail;
                    if (!freeRanges.insert(std::move(remainder)).inserted) throw std::logic_error("GPU free address ranges overlap");
                }
                else freeRanges.erase(it);
            }
            catch (...)
            {
                allocations.erase(inserted.first);
                throw;
            }
            return reinterpret_cast<void*>(aligned);
        }
        throw std::runtime_error("GPU shader address aperture is exhausted");
    }

    void GpuAddressSpace::Release(void* address, std::size_t bytes)
    {
        const auto pointer = reinterpret_cast<std::uintptr_t>(address);
        if (pointer < base || pointer - base >= size) throw std::invalid_argument("GPU address is outside its reserved aperture");
        const auto offset = pointer - base;
        const std::lock_guard lock(mutex);
        const auto allocated = allocations.find(offset);
        if (allocated == allocations.end() || allocated->second != bytes) throw std::invalid_argument("GPU address release does not match an allocation");
        auto next = freeRanges.lower_bound(offset);
        auto previous = next == freeRanges.begin() ? freeRanges.end() : std::prev(next);
        const bool joinPrevious = previous != freeRanges.end() && previous->first + previous->second == offset;
        const bool joinNext = next != freeRanges.end() && offset + bytes == next->first;
        if (joinPrevious)
        {
            previous->second += bytes;
            if (joinNext)
            {
                previous->second += next->second;
                freeRanges.erase(next);
            }
        }
        else if (joinNext)
        {
            auto joined = freeRanges.extract(next);
            joined.key() = offset;
            joined.mapped() += bytes;
            if (!freeRanges.insert(std::move(joined)).inserted) throw std::logic_error("GPU free address ranges overlap");
        }
        else if (!freeRanges.emplace(offset, bytes).second) throw std::logic_error("GPU free address ranges overlap");
        allocations.erase(allocated);
    }
}
