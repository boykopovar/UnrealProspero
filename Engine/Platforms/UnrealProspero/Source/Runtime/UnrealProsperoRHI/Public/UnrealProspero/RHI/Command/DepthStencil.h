#ifndef UNREALPROSPERO_RHI_DEPTH_STENCIL_H
#define UNREALPROSPERO_RHI_DEPTH_STENCIL_H

#include "UnrealProspero/RHI/State/DepthStencilState.h"
#include "SceTypes.hpp"

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API void WriteDepthStencilRegisters(CommandBuffer& commands, const DepthStencilState& state, uint32 reference, bool hasDepth, bool hasStencil, bool depthBounds);
}

#endif
