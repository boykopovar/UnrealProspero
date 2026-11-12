#include "UnrealProspero/Kernel/SystemConfiguration.h"
#include "UnrealProspero/Kernel/PosixResult.h"
#include "prx/libkernel/include/Api.hpp"
#include <bit>
#include <limits>
#include <stdexcept>
#include <string>

namespace UnrealProspero::Kernel
{
    namespace
    {
        template<typename TValue>
        TValue read(const char* name)
        {
            TValue value{};
            std::size_t size = sizeof(value);
            CheckPosixResult(sysctlbyname_nid_postfix(name, &value, &size, nullptr, 0), name);
            if (size != sizeof(value)) throw std::runtime_error(std::string("Unexpected sysctl result size: ") + name);
            return value;
        }
    }

    int SystemConfiguration::LogicalProcessorCount()
    {
        const int count = read<int>("hw.ncpu");
        if (count <= 0) throw std::runtime_error("Invalid logical processor count");
        return count;
    }

    std::size_t SystemConfiguration::PageSize()
    {
        const int size = read<int>("hw.pagesize");
        if (size <= 0 || !std::has_single_bit(static_cast<unsigned int>(size))) throw std::runtime_error("Invalid system page size");
        return static_cast<std::size_t>(size);
    }

    std::uint64_t SystemConfiguration::PhysicalMemorySize()
    {
        constexpr int physicalPageCount = 121;
        const auto pages = sysconf_nid_postfix(physicalPageCount);
        if (pages <= 0) throw std::runtime_error("Invalid physical page count");
        const auto pageSize = PageSize();
        const auto count = static_cast<std::uint64_t>(pages);
        if (count > std::numeric_limits<std::uint64_t>::max() / pageSize) throw std::overflow_error("Physical memory capacity exceeds the byte counter range");
        return count * pageSize;
    }
}
