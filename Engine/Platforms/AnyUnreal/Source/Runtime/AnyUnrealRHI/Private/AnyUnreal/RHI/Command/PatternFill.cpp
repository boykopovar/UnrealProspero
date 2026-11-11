#include "AnyUnreal/RHI/Command/PatternFill.h"
#include "AnyUnreal/RHI/Command/BufferCopy.h"
#include "AnyUnreal/RHI/Command/MemoryAcquire.h"
#include "prx/libSceAgc/DcbFlow/include/Memory.hpp"
#include <algorithm>
#include <stdexcept>

namespace AnyUnreal::RHI
{
    void WritePatternFill(CommandBuffer& commands, void* destination, std::size_t size, std::span<const std::uint32_t> pattern)
    {
        const auto address = reinterpret_cast<std::uintptr_t>(destination);
        constexpr auto addressLimit = std::uint64_t{1} << 48;
        if (!destination || address % 4 != 0 || size == 0 || pattern.empty() || pattern.size() > 4 || !pattern.data() || size % pattern.size_bytes() != 0) throw std::invalid_argument("GPU pattern fill requires an aligned range of complete elements and one to four words");
        if (address >= addressLimit || size > addressLimit - address) throw std::out_of_range("GPU pattern fill exceeds GPU address space");
        const GpuMemoryRange ranges[]{{destination, size}};
        WriteMemoryAcquire(commands, ranges);
        sceAgcDcbWriteData(&commands, 5, 0, address, pattern.data(), static_cast<std::uint32_t>(pattern.size()), 0, 1);
        auto written = pattern.size_bytes();
        while (written < size)
        {
            const auto count = std::min(written, size - written);
            WriteBufferCopy(commands, static_cast<std::byte*>(destination) + written, destination, count);
            written += count;
        }
        WriteMemoryAcquire(commands, ranges);
    }
}
