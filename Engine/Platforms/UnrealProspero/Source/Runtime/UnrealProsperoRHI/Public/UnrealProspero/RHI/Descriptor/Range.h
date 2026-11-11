#ifndef UNREALPROSPERO_RHI_DESCRIPTOR_RANGE_H
#define UNREALPROSPERO_RHI_DESCRIPTOR_RANGE_H

#include <cstddef>
#include <cstdint>
#include <span>

namespace UnrealProspero::RHI
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
