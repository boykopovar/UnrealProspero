#ifndef UNREALPROSPERO_RHI_DEPTH_BIAS_H
#define UNREALPROSPERO_RHI_DEPTH_BIAS_H

#include "SceTypes.hpp"

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API void WriteDepthBias(CommandBuffer& commands, float constant, float slope);
}

#endif
