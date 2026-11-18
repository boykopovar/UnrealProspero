#include "UnrealProspero/Kernel/ProcessMemory.h"
#include "UnrealProspero/Kernel/Debug.h"
#include "UnrealProspero/Kernel/PosixResult.h"
#include "UnrealProspero/Kernel/SystemConfiguration.h"
#include "UnrealProspero/Memory/SystemAllocator.h"
#include "prx/libkernel/include/Api.hpp"
#include <cstring>
#include <limits>
#include <span>
#include <vector>

namespace UnrealProspero::Kernel
{
    namespace
    {
        template<typename TValue>
        TValue read(std::span<const std::byte> record, std::size_t offset)
        {
            if (offset > record.size() || sizeof(TValue) > record.size() - offset) throw std::runtime_error("Truncated process memory record");
            TValue value{};
            std::memcpy(&value, record.data() + offset, sizeof(value));
            return value;
        }

        std::uint64_t bytes(std::int64_t count, std::uint64_t unit)
        {
            if (count < 0) throw std::runtime_error("Negative process memory count");
            if (static_cast<std::uint64_t>(count) > std::numeric_limits<std::uint64_t>::max() / unit) throw std::overflow_error("Process memory exceeds the byte counter range");
            return static_cast<std::uint64_t>(count) * unit;
        }
        auto queryProcessRecord()
        {
            const int process = getpid_nid_postfix();
            const int mib[] = {1, 14, 1, process};
            std::size_t size = 0;
            CheckPosixResult(sysctl_nid_postfix(mib, 4, nullptr, &size, nullptr, 0), "sysctl process record size");
            if (size < 272 || size > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) throw std::runtime_error("Invalid process record size");
            std::vector<std::byte, Memory::SystemAllocator<std::byte>> storage(size);
            CheckPosixResult(sysctl_nid_postfix(mib, 4, storage.data(), &size, nullptr, 0), "sysctl process record");
            if (size > storage.size()) throw std::runtime_error("Process record exceeds its buffer");
            storage.resize(size);
            const auto recordSize = read<std::int32_t>(storage, 0);
            if (recordSize <= 0 || static_cast<std::size_t>(recordSize) != size || read<std::int32_t>(storage, 72) != process) throw std::runtime_error("Unexpected process record");
            return storage;
        }
    }

    bool IsDebuggerAttached()
    {
        constexpr std::size_t processFlagsOffset = 368;
        constexpr std::uint64_t traced = 0x800;
        return (read<std::uint64_t>(queryProcessRecord(), processFlagsOffset) & traced) != 0;
    }

    ProcessMemory ProcessMemory::Query()
    {
        const auto record = queryProcessRecord();
        ProcessMemory result{};
        result.VirtualBytes = read<std::uint64_t>(record, 256);
        result.ResidentBytes = bytes(read<std::int64_t>(record, 264), SystemConfiguration::PageSize());
        GuestResourceUsage usage{};
        CheckPosixResult(getrusage_nid_postfix(0, &usage), "getrusage process memory");
        result.PeakResidentBytes = bytes(usage.ru_maxrss, 1024);
        return result;
    }
}
