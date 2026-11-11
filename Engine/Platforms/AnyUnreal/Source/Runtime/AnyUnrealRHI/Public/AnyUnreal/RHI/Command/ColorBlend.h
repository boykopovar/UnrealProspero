#ifndef ANYUNREAL_RHI_COLOR_BLEND_H
#define ANYUNREAL_RHI_COLOR_BLEND_H

#include "AnyUnreal/RHI/State/BlendState.h"
#include "SceTypes.hpp"

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API void WriteColorBlendRegisters(CommandBuffer& commands, const BlendState& state, uint32 targetCount);
}

#endif
