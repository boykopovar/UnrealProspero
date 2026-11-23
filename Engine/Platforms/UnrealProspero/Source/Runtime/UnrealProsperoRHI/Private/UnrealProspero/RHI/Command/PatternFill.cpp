#include "UnrealProspero/RHI/Command/PatternFill.h"
#include "UnrealProspero/RHI/Command/BufferCopy.h"
#include "UnrealProspero/RHI/Command/MemoryAcquire.h"
#include "prx/libSceAgc/DcbFlow/include/Memory.hpp"
#include <algorithm>
#include <stdexcept>

namespace UnrealProspero::RHI
{
    namespace
    {
        void validateRange(const void* address, std::size_t size)
        {
            const auto value = reinterpret_cast<std::uintptr_t>(address);
            constexpr auto addressLimit = std::uint64_t{1} << 48;
            if (!address || size == 0) throw std::invalid_argument("GPU pattern range must not be empty");
            if (value >= addressLimit || size > addressLimit - value) throw std::out_of_range("GPU pattern range exceeds GPU address space");
        }

        void repeatPattern(CommandBuffer& commands, void* destination, std::size_t size, std::size_t written)
        {
            while (written < size)
            {
                const auto count = std::min(written, size - written);
                WriteBufferCopy(commands, static_cast<std::byte*>(destination) + written, destination, count);
                written += count;
            }
        }
    }

    void WritePatternCopy(CommandBuffer& commands, void* destination, std::size_t size, const void* pattern, std::size_t patternSize)
    {
        if (patternSize == 0 || patternSize > 16 || size % patternSize != 0) throw std::invalid_argument("GPU pattern copy requires complete elements of one to sixteen bytes");
        validateRange(destination, size);
        validateRange(pattern, patternSize);
        const auto output = reinterpret_cast<std::uintptr_t>(destination);
        const auto input = reinterpret_cast<std::uintptr_t>(pattern);
        if (output < input + patternSize && input < output + size) throw std::invalid_argument("GPU pattern copy ranges overlap");
        const GpuMemoryRange ranges[]{{pattern, patternSize}, {destination, size}};
        WriteMemoryAcquire(commands, ranges);
        WriteBufferCopy(commands, destination, pattern, patternSize);
        repeatPattern(commands, destination, size, patternSize);
        WriteMemoryAcquire(commands, std::span(ranges).last(1));
    }

    void WritePatternFill(CommandBuffer& commands, void* destination, std::size_t size, std::span<const std::uint32_t> pattern)
    {
        validateRange(destination, size);
        const auto address = reinterpret_cast<std::uintptr_t>(destination);
        if (address % 4 != 0 || pattern.empty() || pattern.size() > 4 || !pattern.data() || size % pattern.size_bytes() != 0) throw std::invalid_argument("GPU pattern fill requires an aligned range of complete elements and one to four words");
        const GpuMemoryRange ranges[]{{destination, size}};
        WriteMemoryAcquire(commands, ranges);
        sceAgcDcbWriteData(&commands, 5, 0, address, pattern.data(), static_cast<std::uint32_t>(pattern.size()), 0, 1);
        repeatPattern(commands, destination, size, pattern.size_bytes());
        WriteMemoryAcquire(commands, ranges);
    }
}
