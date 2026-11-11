#ifndef ANYUNREAL_RHI_COMPUTE_DISPATCH_H
#define ANYUNREAL_RHI_COMPUTE_DISPATCH_H

#include "AnyUnreal/RHI/Shader/ShaderCommands.h"

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API void WriteComputeDispatch(CommandBuffer& commands, const AgcShader& shader, const Shader::UserDataLayout& layout, const Shader::UserDataInputs& inputs, std::uint32_t groupX, std::uint32_t groupY, std::uint32_t groupZ);
    ANYUNREALRHI_API void WriteIndirectComputeDispatch(CommandBuffer& commands, const AgcShader& shader, const Shader::UserDataLayout& layout, const Shader::UserDataInputs& inputs, const void* argumentBuffer, std::size_t argumentBufferSize, std::uint32_t argumentOffset);
}

#endif
