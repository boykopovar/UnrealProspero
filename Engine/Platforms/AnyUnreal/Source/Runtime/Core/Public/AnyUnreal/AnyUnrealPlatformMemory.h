#ifndef ANYUNREAL_PLATFORM_MEMORY_H
#define ANYUNREAL_PLATFORM_MEMORY_H

#include "GenericPlatform/GenericPlatformMemory.h"
#include "AnyUnreal/Memory/PageAllocator.h"
#include <stdexcept>

struct FPlatformMemoryStats : FGenericPlatformMemoryStats
{
};

struct FAnyUnrealPlatformMemory : FGenericPlatformMemory
{
    static void* BinnedAllocFromOS(SIZE_T size)
    {
        return AnyUnreal::Memory::PageAllocator::Allocate(size);
    }

    static void BinnedFreeToOS(void* address, SIZE_T size)
    {
        AnyUnreal::Memory::PageAllocator::Free(address, size);
    }

    static bool PageProtect(void* address, SIZE_T size, bool readable, bool writable)
    {
        AnyUnreal::Memory::PageAllocator::Protect(address, size, readable, writable);
        return true;
    }

    static FPlatformMemoryStats GetStats()
    {
        throw std::runtime_error("AnyUnreal process memory statistics are not implemented");
    }

    static const FPlatformMemoryConstants& GetConstants()
    {
        throw std::runtime_error("AnyUnreal memory capacity reporting is not implemented");
    }
};

using FPlatformMemory = FAnyUnrealPlatformMemory;

#endif
