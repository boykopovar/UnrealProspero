#ifndef UNREALPROSPERO_MEMORY_PAGE_ALLOCATOR_H
#define UNREALPROSPERO_MEMORY_PAGE_ALLOCATOR_H

#include <cstddef>

namespace UnrealProspero::Memory
{
    class PageAllocator
    {
    public:
        static constexpr std::size_t Alignment = 65536;
        static void* Allocate(std::size_t size);
        static void Free(void* address, std::size_t size);
        static void Protect(void* address, std::size_t size, bool readable, bool writable);
    };
}

#endif
