#ifndef UNREALPROSPERO_RHI_COMPUTE_DISPATCH_H
#define UNREALPROSPERO_RHI_COMPUTE_DISPATCH_H

#include "UnrealProspero/RHI/Shader/ShaderCommands.h"

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API void WriteComputeDispatch(CommandBuffer& commands, const AgcShader& shader, const Shader::UserDataLayout& layout, const Shader::UserDataInputs& inputs, std::uint32_t groupX, std::uint32_t groupY, std::uint32_t groupZ);
    UNREALPROSPERORHI_API void WriteIndirectComputeDispatch(CommandBuffer& commands, const AgcShader& shader, const Shader::UserDataLayout& layout, const Shader::UserDataInputs& inputs, const void* argumentBuffer, std::size_t argumentBufferSize, std::uint32_t argumentOffset);
}

#endif
