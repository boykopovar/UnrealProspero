#include "UnrealProspero/RHI/Command/BufferCopy.h"
#include "prx/libSceAgc/DcbFlow/include/Memory.hpp"
#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace UnrealProspero::RHI
{
    void WriteBufferCopy(CommandBuffer& commands, void* destination, const void* source, std::size_t size)
    {
        auto destinationAddress = reinterpret_cast<std::uintptr_t>(destination);
        auto sourceAddress = reinterpret_cast<std::uintptr_t>(source);
        constexpr auto addressLimit = std::uint64_t{1} << 48;
        if (!destination || !source || size == 0) throw std::invalid_argument("GPU buffer copy requires nonempty source and destination ranges");
        if (destinationAddress >= addressLimit || sourceAddress >= addressLimit || size > addressLimit - destinationAddress || size > addressLimit - sourceAddress) throw std::out_of_range("GPU buffer copy exceeds GPU address space");
        if (destinationAddress < sourceAddress + size && sourceAddress < destinationAddress + size) throw std::invalid_argument("GPU buffer copy ranges overlap");
        while (size != 0)
        {
            const auto count = static_cast<std::uint32_t>(std::min(size, std::size_t{0x3fffffc}));
            sceAgcDcbDmaData(&commands, 0, 0, 0, destinationAddress, 0, 0, sourceAddress, count, 1, 0, 1);
            destinationAddress += count;
            sourceAddress += count;
            size -= count;
        }
    }
}
