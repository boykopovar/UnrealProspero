#ifndef ANYUNREAL_RHI_DEPTH_BOUNDS_H
#define ANYUNREAL_RHI_DEPTH_BOUNDS_H

#include "SceTypes.hpp"

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API void WriteDepthBounds(CommandBuffer& commands, float minDepth, float maxDepth);
}

#endif
