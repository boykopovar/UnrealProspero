#ifndef UNREALPROSPERO_RHI_PATTERN_FILL_H
#define UNREALPROSPERO_RHI_PATTERN_FILL_H

#include "SceTypes.hpp"
#include <cstddef>
#include <cstdint>
#include <span>

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API void WritePatternCopy(CommandBuffer& commands, void* destination, std::size_t size, const void* pattern, std::size_t patternSize);
    UNREALPROSPERORHI_API void WritePatternFill(CommandBuffer& commands, void* destination, std::size_t size, std::span<const std::uint32_t> pattern);
}

#endif
