#ifndef ANYUNREAL_RHI_MEMORY_HALF_FLOAT_H
#define ANYUNREAL_RHI_MEMORY_HALF_FLOAT_H

#include <cstdint>

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API std::uint16_t EncodeHalfFloat(float value);
    ANYUNREALRHI_API float DecodeHalfFloat(std::uint16_t value);
}

#endif
