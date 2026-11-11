#include "AnyUnreal/RHI/Descriptor/BufferDescriptor.h"
#include <limits>
#include <stdexcept>

namespace AnyUnreal::RHI
{
    std::array<std::uint32_t, 4> CreateByteAddressBufferDescriptor(const void* address, std::size_t size)
    {
        const auto base = reinterpret_cast<std::uintptr_t>(address);
        constexpr std::uint64_t addressLimit = std::uint64_t{1} << 48;
        if (base == 0 || base % 4 != 0 || base >= addressLimit) throw std::invalid_argument("Invalid byte-address buffer address");
        if (size == 0 || size % 4 != 0 || size > std::numeric_limits<std::uint32_t>::max() || size > addressLimit - base) throw std::invalid_argument("Invalid byte-address buffer size");
        return {static_cast<std::uint32_t>(base), static_cast<std::uint32_t>(base >> 32), static_cast<std::uint32_t>(size), 0x31016fac};
    }
}
