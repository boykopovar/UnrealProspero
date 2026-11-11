#ifndef UNREALPROSPERO_RHI_INDIRECT_DRAW_H
#define UNREALPROSPERO_RHI_INDIRECT_DRAW_H

#include "UnrealProspero/Shader/UserData.h"
#include "UnrealProspero/RHI/Memory/BufferAllocation.h"
#include "SceTypes.hpp"

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API void WriteIndirectDraw(CommandBuffer& commands, const Shader::UserDataLayout& layout, const BufferAllocation& arguments, std::uint32_t argumentOffset, const BufferAllocation* indices, std::uint32_t indexStride);
}

#endif
