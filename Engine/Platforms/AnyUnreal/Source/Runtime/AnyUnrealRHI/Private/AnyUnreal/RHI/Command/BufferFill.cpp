#include "AnyUnreal/RHI/Command/BufferFill.h"
#include "prx/libSceAgc/DcbFlow/include/Memory.hpp"
#include <algorithm>
#include <stdexcept>

namespace AnyUnreal::RHI
{
    void WriteBufferFill(CommandBuffer& commands, void* destination, std::size_t size, std::uint32_t value)
    {
        auto address = reinterpret_cast<std::uintptr_t>(destination);
        constexpr auto addressLimit = std::uint64_t{1} << 48;
        if (!destination || address % 4 != 0 || size == 0 || size % 4 != 0) throw std::invalid_argument("GPU buffer fill requires a nonempty aligned range");
        if (address >= addressLimit || size > addressLimit - address) throw std::out_of_range("GPU buffer fill exceeds GPU address space");
        while (size != 0)
        {
            const auto count = static_cast<std::uint32_t>(std::min(size, std::size_t{0x3fffffc}));
            sceAgcDcbDmaData(&commands, 0, 0, 0, address, 2, 0, value, count, 1, 0, 1);
            address += count;
            size -= count;
        }
    }
}
