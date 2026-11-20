#ifndef UNREALPROSPERO_RHI_MEMORY_REUSABLE_ALLOCATION_H
#define UNREALPROSPERO_RHI_MEMORY_REUSABLE_ALLOCATION_H

#include "UnrealProspero/Kernel/DirectAllocation.h"
#include <memory>

namespace UnrealProspero::RHI
{
    class ReusableAllocation final
    {
    public:
        ReusableAllocation(std::size_t size, std::uint32_t addressHigh, std::size_t alignment);
        ~ReusableAllocation() noexcept(false);
        ReusableAllocation(const ReusableAllocation&) = delete;
        ReusableAllocation& operator=(const ReusableAllocation&) = delete;
        void* Data() const;
        std::size_t Size() const;

    private:
        std::unique_ptr<Kernel::DirectAllocation> allocation;
        std::uint32_t addressHigh;
        std::size_t alignment;
    };
}

#endif
