#ifndef ANYUNREAL_RHI_PATTERN_FILL_H
#define ANYUNREAL_RHI_PATTERN_FILL_H

#include "SceTypes.hpp"
#include <cstddef>
#include <cstdint>
#include <span>

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API void WritePatternFill(CommandBuffer& commands, void* destination, std::size_t size, std::span<const std::uint32_t> pattern);
}

#endif
