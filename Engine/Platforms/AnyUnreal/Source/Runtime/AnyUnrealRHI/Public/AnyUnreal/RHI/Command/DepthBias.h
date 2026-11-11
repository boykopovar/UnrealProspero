#ifndef ANYUNREAL_RHI_DEPTH_BIAS_H
#define ANYUNREAL_RHI_DEPTH_BIAS_H

#include "SceTypes.hpp"

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API void WriteDepthBias(CommandBuffer& commands, float constant, float slope);
}

#endif
