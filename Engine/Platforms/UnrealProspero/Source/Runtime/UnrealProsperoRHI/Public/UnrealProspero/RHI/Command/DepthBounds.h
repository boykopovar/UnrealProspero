#ifndef UNREALPROSPERO_RHI_DEPTH_BOUNDS_H
#define UNREALPROSPERO_RHI_DEPTH_BOUNDS_H

#include "SceTypes.hpp"

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API void WriteDepthBounds(CommandBuffer& commands, float minDepth, float maxDepth);
}

#endif
