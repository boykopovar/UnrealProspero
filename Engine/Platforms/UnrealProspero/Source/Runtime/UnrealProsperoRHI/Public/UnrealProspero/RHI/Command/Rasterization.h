#ifndef UNREALPROSPERO_RHI_RASTERIZATION_H
#define UNREALPROSPERO_RHI_RASTERIZATION_H

#include "UnrealProspero/RHI/State/RasterizerState.h"
#include "SceTypes.hpp"

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API void WriteRasterizationRegisters(CommandBuffer& commands, const RasterizerState& state, uint32 modeControl, uint32 clipControl, bool depthBias);
}

#endif
