#ifndef ANYUNREAL_RHI_INDIRECT_DRAW_H
#define ANYUNREAL_RHI_INDIRECT_DRAW_H

#include "AnyUnreal/Shader/UserData.h"
#include "AnyUnreal/RHI/Memory/BufferAllocation.h"
#include "SceTypes.hpp"

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API void WriteIndirectDraw(CommandBuffer& commands, const Shader::UserDataLayout& layout, const BufferAllocation& arguments, std::uint32_t argumentOffset, const BufferAllocation* indices, std::uint32_t indexStride);
}

#endif
