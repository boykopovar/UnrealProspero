#ifndef ANYUNREAL_RHI_BUFFER_FILL_H
#define ANYUNREAL_RHI_BUFFER_FILL_H

#include "SceTypes.hpp"
#include <cstddef>
#include <cstdint>

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API void WriteBufferFill(CommandBuffer& commands, void* destination, std::size_t size, std::uint32_t value);
}

#endif
