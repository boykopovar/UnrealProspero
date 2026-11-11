#ifndef ANYUNREAL_RHI_BUFFER_CLEAR_VALUE_H
#define ANYUNREAL_RHI_BUFFER_CLEAR_VALUE_H

#include "AnyUnreal/RHI/Descriptor/BufferDescriptor.h"

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API std::array<std::uint32_t, 4> EncodeBufferClear(BufferFormat format, const std::array<std::uint32_t, 4>& values);
    ANYUNREALRHI_API std::array<std::uint32_t, 4> EncodeBufferClear(BufferFormat format, const std::array<float, 4>& values);
}

#endif
