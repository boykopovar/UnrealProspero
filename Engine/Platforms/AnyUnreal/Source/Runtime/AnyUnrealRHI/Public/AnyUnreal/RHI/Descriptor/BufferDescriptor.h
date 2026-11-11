#ifndef ANYUNREAL_RHI_DESCRIPTOR_BUFFER_H
#define ANYUNREAL_RHI_DESCRIPTOR_BUFFER_H

#include <array>
#include <cstddef>
#include <cstdint>

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API std::array<std::uint32_t, 4> CreateByteAddressBufferDescriptor(const void* address, std::size_t size);
    ANYUNREALRHI_API std::array<std::uint32_t, 4> CreateVertexBufferDescriptor(const void* address, std::size_t size, std::uint32_t stride);
}

#endif
