#include "UnrealProspero/RHI/Descriptor/BufferDescriptor.h"
#include <limits>
#include <algorithm>
#include <stdexcept>

namespace UnrealProspero::RHI
{
    namespace
    {
        struct ElementFormat
        {
            std::uint32_t Encoding;
            std::uint32_t Size;
            std::uint32_t Swizzle;
        };

        ElementFormat describe(BufferFormat format)
        {
            switch (format)
            {
            case BufferFormat::Float1: return {22, 4, 0x204};
            case BufferFormat::Float2: return {64, 8, 0x22c};
            case BufferFormat::Float3: return {74, 12, 0x3ac};
            case BufferFormat::Float4: return {77, 16, 0xfac};
            case BufferFormat::Byte4Snorm: return {57, 4, 0xfac};
            case BufferFormat::Byte4Uint: return {60, 4, 0xfac};
            case BufferFormat::Byte4Unorm: return {56, 4, 0xfac};
            case BufferFormat::Bgra8Unorm: return {56, 4, 0xf2e};
            case BufferFormat::Short2Sint: return {28, 4, 0x22c};
            case BufferFormat::Short4Sint: return {70, 8, 0xfac};
            case BufferFormat::Short2Snorm: return {24, 4, 0x22c};
            case BufferFormat::Half2: return {29, 4, 0x22c};
            case BufferFormat::Half4: return {71, 8, 0xfac};
            case BufferFormat::Short4Snorm: return {66, 8, 0xfac};
            case BufferFormat::Short2Uint: return {27, 4, 0x22c};
            case BufferFormat::Short4Uint: return {69, 8, 0xfac};
            case BufferFormat::Short2Unorm: return {23, 4, 0x22c};
            case BufferFormat::Short4Unorm: return {65, 8, 0xfac};
            case BufferFormat::Rgb10A2Unorm: return {44, 4, 0xfac};
            case BufferFormat::Uint1: return {20, 4, 0x204};
            case BufferFormat::Uint2: return {62, 8, 0x22c};
            case BufferFormat::Uint3: return {72, 12, 0x3ac};
            case BufferFormat::Uint4: return {75, 16, 0xfac};
            case BufferFormat::Sint1: return {21, 4, 0x204};
            case BufferFormat::Sint3: return {73, 12, 0x3ac};
            default: throw std::invalid_argument("Invalid typed buffer format");
            }
        }

        std::uintptr_t checkRange(const void* address, std::size_t size)
        {
            const auto base = reinterpret_cast<std::uintptr_t>(address);
            constexpr std::uint64_t addressLimit = std::uint64_t{1} << 48;
            if (base == 0 || base >= addressLimit) throw std::invalid_argument("Invalid buffer descriptor address");
            if (size == 0 || size > std::numeric_limits<std::uint32_t>::max() || size > addressLimit - base) throw std::invalid_argument("Invalid buffer descriptor size");
            return base;
        }
    }

    std::uint32_t GetBufferElementSize(BufferFormat format) { return describe(format).Size; }

    std::array<std::uint32_t, 4> CreateTypedBufferDescriptor(const void* address, std::size_t size, BufferFormat format)
    {
        const auto base = checkRange(address, size);
        const auto elementSize = GetBufferElementSize(format);
        if (base % std::min(elementSize, 4u) != 0 || size % elementSize != 0) throw std::invalid_argument("Typed buffer view requires complete aligned elements");
        return CreateTypedVertexBufferDescriptor(address, size, elementSize, format);
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
    std::array<std::uint32_t, 4> CreateTypedVertexBufferDescriptor(const void* address, std::size_t size, std::uint32_t stride, BufferFormat format)
    {
        const auto base = checkRange(address, size);
        if (stride > 0x3fff) throw std::out_of_range("Vertex stride exceeds the descriptor field");
        const auto [encoding, elementSize, swizzle] = describe(format);
        if (size < elementSize) throw std::out_of_range("Vertex buffer does not contain a complete element");
        const auto records = stride == 0 ? size : (size - elementSize) / stride + 1;
        const auto bounds = stride == 0 ? 3u : 1u;
        return {static_cast<std::uint32_t>(base), static_cast<std::uint32_t>(base >> 32) | (stride << 16), static_cast<std::uint32_t>(records), swizzle | (encoding << 12) | (1u << 24) | (bounds << 28)};
    }

}
