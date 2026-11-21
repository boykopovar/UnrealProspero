#ifndef UNREALPROSPERO_PLATFORM_MEMORY_H
#define UNREALPROSPERO_PLATFORM_MEMORY_H

#include "GenericPlatform/GenericPlatformMemory.h"
#include "UnrealProspero/Memory/PageAllocator.h"
#include "UnrealProspero/Kernel/MemoryConstants.h"
#include "UnrealProspero/Memory/VirtualMemoryBlock.h"
#include <stdexcept>

struct FPlatformMemoryStats : FGenericPlatformMemoryStats
{
};

struct FUnrealProsperoPlatformMemory : FGenericPlatformMemory
{
    static CORE_API class FMalloc* BaseAllocator();
    using FPlatformVirtualMemoryBlock = UnrealProspero::Memory::VirtualMemoryBlock;
    static void* BinnedAllocFromOS(SIZE_T size)
    {
        return UnrealProspero::Memory::PageAllocator::Allocate(size);
    }

    static void BinnedFreeToOS(void* address, SIZE_T size)
    {
        UnrealProspero::Memory::PageAllocator::Free(address, size);
    }

    static bool PageProtect(void* address, SIZE_T size, bool readable, bool writable)
    {
        UnrealProspero::Memory::PageAllocator::Protect(address, size, readable, writable);
        return true;
    }

    static bool GetLLMAllocFunctions(void*(*&allocate)(std::size_t), void(*&free)(void*, std::size_t), int32& alignment)
    {
        allocate = UnrealProspero::Memory::PageAllocator::Allocate;
        free = UnrealProspero::Memory::PageAllocator::Free;
        alignment = UnrealProspero::Kernel::MemoryPageSize;
        return true;
    }

    static FPlatformMemoryStats GetStats();
    static const FPlatformMemoryConstants& GetConstants();

};

using FPlatformMemory = FUnrealProsperoPlatformMemory;

#endif
