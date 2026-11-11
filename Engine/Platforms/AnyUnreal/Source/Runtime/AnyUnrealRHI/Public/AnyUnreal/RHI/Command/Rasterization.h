#ifndef ANYUNREAL_RHI_RASTERIZATION_H
#define ANYUNREAL_RHI_RASTERIZATION_H

#include "AnyUnreal/RHI/State/RasterizerState.h"
#include "SceTypes.hpp"

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API void WriteRasterizationRegisters(CommandBuffer& commands, const RasterizerState& state, uint32 modeControl, uint32 clipControl, bool depthBias);
}

#endif
