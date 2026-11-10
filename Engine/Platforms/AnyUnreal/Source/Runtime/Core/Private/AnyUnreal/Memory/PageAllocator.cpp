#include "AnyUnreal/Memory/PageAllocator.h"
#include "AnyUnreal/Memory/SystemAllocator.h"
#include "AnyUnreal/Kernel/DirectAllocation.h"
#include "AnyUnreal/Mutex/ScopedLock.h"
#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>

namespace AnyUnreal::Memory
{
    namespace
    {
        using Allocation = Kernel::DirectAllocation;
        using Entry = std::pair<const std::uintptr_t, Allocation*>;

        void destroyAllocation(Allocation* allocation)
        {
            try
            {
                std::destroy_at(allocation);
            }
            catch (...)
            {
                SystemAllocator<Allocation>{}.deallocate(allocation, 1);
                throw;
            }
            SystemAllocator<Allocation>{}.deallocate(allocation, 1);
        }

        struct Allocations
        {
            UE::FAnyUnrealRecursiveMutex Mutex;
            std::map<std::uintptr_t, Allocation*, std::less<std::uintptr_t>, SystemAllocator<Entry>> Blocks;

            ~Allocations() noexcept(false)
            {
                for (const auto& entry : Blocks) destroyAllocation(entry.second);
            }
        };

        Allocations& getAllocations()
        {
            static Allocations allocations;
            return allocations;
        }
    }

    void* PageAllocator::Allocate(std::size_t size)
    {
        auto& allocations = getAllocations();
        ScopedLock lock(allocations.Mutex);
        Allocation* allocation = SystemAllocator<Allocation>{}.allocate(1);
        try
        {
            std::construct_at(allocation, size, Alignment);
        }
        catch (...)
        {
            SystemAllocator<Allocation>{}.deallocate(allocation, 1);
            throw;
        }
        try
        {
            if (!allocations.Blocks.emplace(reinterpret_cast<std::uintptr_t>(allocation->Data()), allocation).second) throw std::runtime_error("Duplicate page allocation address");
        }
        catch (...)
        {
            destroyAllocation(allocation);
            throw;
        }
        return allocation->Data();
    }

    void PageAllocator::Free(void* address, std::size_t size)
    {
        auto& allocations = getAllocations();
        ScopedLock lock(allocations.Mutex);
        auto entry = allocations.Blocks.find(reinterpret_cast<std::uintptr_t>(address));
        if (entry == allocations.Blocks.end() || entry->second->Size() != size) throw std::invalid_argument("Unknown page allocation or mismatched size");
        auto removed = allocations.Blocks.extract(entry);
        destroyAllocation(removed.mapped());
    }

    void PageAllocator::Protect(void* address, std::size_t size, bool readable, bool writable)
    {
        auto& allocations = getAllocations();
        ScopedLock lock(allocations.Mutex);
        const auto location = reinterpret_cast<std::uintptr_t>(address);
        auto entry = allocations.Blocks.upper_bound(location);
        if (entry == allocations.Blocks.begin()) throw std::invalid_argument("Unknown page protection address");
        --entry;
        entry->second->ProtectCpu(location - entry->first, size, readable, writable);
    }
}
