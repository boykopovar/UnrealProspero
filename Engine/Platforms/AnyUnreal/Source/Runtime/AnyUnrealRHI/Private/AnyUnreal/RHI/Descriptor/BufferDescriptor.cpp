#include "AnyUnreal/RHI/Descriptor/BufferDescriptor.h"
#include <limits>
#include <stdexcept>

namespace AnyUnreal::RHI
{
    namespace
    {
        std::uintptr_t checkRange(const void* address, std::size_t size)
        {
            const auto base = reinterpret_cast<std::uintptr_t>(address);
            constexpr std::uint64_t addressLimit = std::uint64_t{1} << 48;
            if (base == 0 || base >= addressLimit) throw std::invalid_argument("Invalid buffer descriptor address");
            if (size == 0 || size > std::numeric_limits<std::uint32_t>::max() || size > addressLimit - base) throw std::invalid_argument("Invalid buffer descriptor size");
            return base;
        }
    }

    std::array<std::uint32_t, 4> CreateByteAddressBufferDescriptor(const void* address, std::size_t size)
    {
        const auto base = checkRange(address, size);
        if (base % 4 != 0 || size % 4 != 0) throw std::invalid_argument("Byte-address buffer descriptor requires word alignment");
        return {static_cast<std::uint32_t>(base), static_cast<std::uint32_t>(base >> 32), static_cast<std::uint32_t>(size), 0x31016fac};
    }

    std::array<std::uint32_t, 4> CreateVertexBufferDescriptor(const void* address, std::size_t size, std::uint32_t stride)
    {
        const auto base = checkRange(address, size);
        if (stride > 0x3fff) throw std::out_of_range("Vertex stride exceeds the descriptor field");
        const auto records = stride == 0 ? size : (size - 1) / stride + 1;
        return {static_cast<std::uint32_t>(base), static_cast<std::uint32_t>(base >> 32) | (stride << 16), static_cast<std::uint32_t>(records), stride == 0 ? 0x31016facu : 0x01016facu};
    }
}
