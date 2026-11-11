#ifndef ANYUNREAL_RHI_BUFFER_COPY_H
#define ANYUNREAL_RHI_BUFFER_COPY_H

#include "SceTypes.hpp"
#include <cstddef>

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API void WriteBufferCopy(CommandBuffer& commands, void* destination, const void* source, std::size_t size);
}

#endif
