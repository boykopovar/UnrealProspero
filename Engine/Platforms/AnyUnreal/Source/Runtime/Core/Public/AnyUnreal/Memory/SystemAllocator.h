#ifndef ANYUNREAL_MEMORY_SYSTEM_ALLOCATOR_H
#define ANYUNREAL_MEMORY_SYSTEM_ALLOCATOR_H

#include <cstddef>
#include <cstdlib>
#include <limits>
#include <new>

namespace AnyUnreal::Memory
{
    template<typename TValue>
    class SystemAllocator
    {
    public:
        using value_type = TValue;

        SystemAllocator() noexcept = default;

        template<typename TOther>
        SystemAllocator(const SystemAllocator<TOther>&) noexcept
        {
        }

        TValue* allocate(std::size_t count)
        {
            static_assert(alignof(TValue) <= alignof(std::max_align_t));
            if (count > std::numeric_limits<std::size_t>::max() / sizeof(TValue)) throw std::bad_array_new_length();
            void* address = std::malloc(count == 0 ? 1 : count * sizeof(TValue));
            if (address == nullptr) throw std::bad_alloc();
            return static_cast<TValue*>(address);
        }

        void deallocate(TValue* address, std::size_t) noexcept
        {
            std::free(address);
        }

        template<typename TOther>
        bool operator==(const SystemAllocator<TOther>&) const noexcept
        {
            return true;
        }
    };
}

#endif
