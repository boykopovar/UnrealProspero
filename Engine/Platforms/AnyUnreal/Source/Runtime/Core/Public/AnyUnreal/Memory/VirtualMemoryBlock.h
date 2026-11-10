#ifndef ANYUNREAL_MEMORY_VIRTUAL_MEMORY_BLOCK_H
#define ANYUNREAL_MEMORY_VIRTUAL_MEMORY_BLOCK_H

#include "GenericPlatform/GenericPlatformMemory.h"
#include <cstddef>

namespace AnyUnreal::Memory
{
    class VirtualMemoryBlock : public FGenericPlatformMemory::FBasicVirtualMemoryBlock
    {
    public:
        VirtualMemoryBlock() = default;
        VirtualMemoryBlock(void* address, uint32 pageCount);
        static constexpr std::size_t GetCommitAlignment() { return 16384; }
        static constexpr std::size_t GetVirtualSizeAlignment() { return GetCommitAlignment(); }
        static VirtualMemoryBlock AllocateVirtual(std::size_t size, std::size_t alignment = GetVirtualSizeAlignment());
        std::size_t GetActualSize() const;
        void Commit(std::size_t offset, std::size_t size);
        void Decommit(std::size_t offset, std::size_t size);
        void Commit();
        void Decommit();
        void CommitByPtr(void* address, std::size_t size);
        void DecommitByPtr(void* address, std::size_t size);
        void FreeVirtual();

    private:
        void validateRange(std::size_t offset, std::size_t size) const;
        std::size_t offsetOf(void* address) const;
    };
}

#endif
