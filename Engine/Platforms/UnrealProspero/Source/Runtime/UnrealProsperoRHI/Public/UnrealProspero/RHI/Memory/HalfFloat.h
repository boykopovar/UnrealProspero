#ifndef UNREALPROSPERO_RHI_MEMORY_HALF_FLOAT_H
#define UNREALPROSPERO_RHI_MEMORY_HALF_FLOAT_H

#include "CoreTypes.h"
#include <cstdint>

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API std::uint16_t EncodeHalfFloat(float value);
    UNREALPROSPERORHI_API float DecodeHalfFloat(std::uint16_t value);
}

#endif
