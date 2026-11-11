#ifndef UNREALPROSPERO_RHI_COLOR_BLEND_H
#define UNREALPROSPERO_RHI_COLOR_BLEND_H

#include "UnrealProspero/RHI/State/BlendState.h"
#include "SceTypes.hpp"

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API void WriteColorBlendRegisters(CommandBuffer& commands, const BlendState& state, uint32 targetCount);
}

#endif
