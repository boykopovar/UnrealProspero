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
    std::array<std::uint32_t, 4> CreateTypedVertexBufferDescriptor(const void* address, std::size_t size, std::uint32_t stride, VertexFormat format)
    {
        const auto base = checkRange(address, size);
        if (stride > 0x3fff) throw std::out_of_range("Vertex stride exceeds the descriptor field");
        std::uint32_t encoding;
        std::uint32_t elementSize;
        std::uint32_t swizzle;
        switch (format)
        {
        case VertexFormat::Float1: encoding = 22; elementSize = 4; swizzle = 0x204; break;
        case VertexFormat::Float2: encoding = 64; elementSize = 8; swizzle = 0x22c; break;
        case VertexFormat::Float3: encoding = 74; elementSize = 12; swizzle = 0x3ac; break;
        case VertexFormat::Float4: encoding = 77; elementSize = 16; swizzle = 0xfac; break;
        case VertexFormat::Byte4Snorm: encoding = 57; elementSize = 4; swizzle = 0xfac; break;
        case VertexFormat::Byte4Uint: encoding = 60; elementSize = 4; swizzle = 0xfac; break;
        case VertexFormat::Byte4Unorm: encoding = 56; elementSize = 4; swizzle = 0xfac; break;
        case VertexFormat::Bgra8Unorm: encoding = 56; elementSize = 4; swizzle = 0xf2e; break;
        case VertexFormat::Short2Sint: encoding = 28; elementSize = 4; swizzle = 0x22c; break;
        case VertexFormat::Short4Sint: encoding = 70; elementSize = 8; swizzle = 0xfac; break;
        case VertexFormat::Short2Snorm: encoding = 24; elementSize = 4; swizzle = 0x22c; break;
        case VertexFormat::Half2: encoding = 29; elementSize = 4; swizzle = 0x22c; break;
        case VertexFormat::Half4: encoding = 71; elementSize = 8; swizzle = 0xfac; break;
        case VertexFormat::Short4Snorm: encoding = 66; elementSize = 8; swizzle = 0xfac; break;
        case VertexFormat::Short2Uint: encoding = 27; elementSize = 4; swizzle = 0x22c; break;
        case VertexFormat::Short4Uint: encoding = 69; elementSize = 8; swizzle = 0xfac; break;
        case VertexFormat::Short2Unorm: encoding = 23; elementSize = 4; swizzle = 0x22c; break;
        case VertexFormat::Short4Unorm: encoding = 65; elementSize = 8; swizzle = 0xfac; break;
        case VertexFormat::Rgb10A2Unorm: encoding = 44; elementSize = 4; swizzle = 0xfac; break;
        case VertexFormat::Uint1: encoding = 20; elementSize = 4; swizzle = 0x204; break;
        default: throw std::invalid_argument("Invalid typed vertex format");
        }
        if (size < elementSize) throw std::out_of_range("Vertex buffer does not contain a complete element");
        const auto records = stride == 0 ? size : (size - elementSize) / stride + 1;
        const auto bounds = stride == 0 ? 3u : 1u;
        return {static_cast<std::uint32_t>(base), static_cast<std::uint32_t>(base >> 32) | (stride << 16), static_cast<std::uint32_t>(records), swizzle | (encoding << 12) | (1u << 24) | (bounds << 28)};
    }

}
