#ifndef ANYUNREAL_RHI_DESCRIPTOR_RANGE_H
#define ANYUNREAL_RHI_DESCRIPTOR_RANGE_H

#include <cstddef>
#include <cstdint>
#include <span>

namespace AnyUnreal::RHI
{
    struct DescriptorRange
    {
        std::uint32_t Set;
        std::uint32_t Binding;
        std::uint32_t Offset;
        std::span<const std::byte> Bytes;
    };
}

#endif
