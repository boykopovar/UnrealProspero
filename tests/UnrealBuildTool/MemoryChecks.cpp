#include "AnyUnreal/Kernel/DirectAllocation.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace
{
    bool checkProtection(void* address, int expected)
    {
        void* start = nullptr;
        void* end = nullptr;
        int protection = 0;
        AnyUnreal::Kernel::CheckResult(sceKernelQueryMemoryProtection(address, &start, &end, &protection), "sceKernelQueryMemoryProtection");
        const auto location = reinterpret_cast<std::uintptr_t>(address);
        return protection == expected && reinterpret_cast<std::uintptr_t>(start) <= location && location < reinterpret_cast<std::uintptr_t>(end);
    }
}

bool CheckMemory()
{
    using AnyUnreal::Kernel::DirectAllocation;
    constexpr auto page = DirectAllocation::PageSize;
    const std::size_t alignments[]{page, 65536, 2097152};
    for (const auto alignment : alignments)
    {
        for (int repetition = 0; repetition < 2; ++repetition)
        {
            DirectAllocation allocation(page * 3, alignment);
            if (allocation.Size() != page * 3 || reinterpret_cast<std::uintptr_t>(allocation.Data()) % alignment != 0) return false;
            auto* bytes = static_cast<unsigned char*>(allocation.Data());
            for (std::size_t i = 0; i < allocation.Size(); ++i)
            {
                if (bytes[i] != 0) return false;
                bytes[i] = static_cast<unsigned char>(i % 251);
            }
            allocation.Protect(page, page, true, false);
            if (!checkProtection(bytes, 3) || !checkProtection(bytes + page, 1) || !checkProtection(bytes + page * 2, 3)) return false;
            for (std::size_t i = 0; i < allocation.Size(); ++i)
            {
                if (bytes[i] != static_cast<unsigned char>(i % 251)) return false;
            }
            allocation.Protect(page, page, false, false);
            if (!checkProtection(bytes + page, 0)) return false;
            allocation.Protect(page, page, true, true);
            bytes[page] = 42;
            if (!checkProtection(bytes + page, 3) || bytes[page] != 42) return false;
            try
            {
                allocation.Protect(page * 2, page * 2, true, true);
                return false;
            }
            catch (const std::invalid_argument&)
            {
            }
        }
    }
    try
    {
        DirectAllocation invalid(page, page * 3);
        return false;
    }
    catch (const std::invalid_argument&)
    {
    }
    try
    {
        DirectAllocation invalid(page - 1);
        return false;
    }
    catch (const std::invalid_argument&)
    {
    }
    return true;
}
