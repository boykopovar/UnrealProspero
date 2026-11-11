#ifndef ANYUNREAL_RHI_DEPTH_STENCIL_H
#define ANYUNREAL_RHI_DEPTH_STENCIL_H

#include "AnyUnreal/RHI/State/DepthStencilState.h"
#include "SceTypes.hpp"

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API void WriteDepthStencilRegisters(CommandBuffer& commands, const DepthStencilState& state, uint32 reference, bool hasDepth, bool hasStencil, bool depthBounds);
}

#endif
