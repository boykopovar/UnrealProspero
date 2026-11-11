#ifndef ANYUNREAL_RHI_DESCRIPTOR_BUFFER_H
#define ANYUNREAL_RHI_DESCRIPTOR_BUFFER_H

#include <array>
#include <cstddef>
#include <cstdint>

namespace AnyUnreal::RHI
{
    enum class VertexFormat { Untyped, Float1, Float2, Float3, Float4, Byte4Snorm, Byte4Uint, Byte4Unorm, Bgra8Unorm, Short2Sint, Short4Sint, Short2Snorm, Half2, Half4, Short4Snorm, Short2Uint, Short4Uint, Short2Unorm, Short4Unorm, Rgb10A2Unorm, Uint1 };

    ANYUNREALRHI_API std::array<std::uint32_t, 4> CreateByteAddressBufferDescriptor(const void* address, std::size_t size);
    ANYUNREALRHI_API std::array<std::uint32_t, 4> CreateVertexBufferDescriptor(const void* address, std::size_t size, std::uint32_t stride);
    ANYUNREALRHI_API std::array<std::uint32_t, 4> CreateTypedVertexBufferDescriptor(const void* address, std::size_t size, std::uint32_t stride, VertexFormat format);
}

#endif
