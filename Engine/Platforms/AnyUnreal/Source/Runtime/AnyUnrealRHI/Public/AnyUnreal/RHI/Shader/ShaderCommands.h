#ifndef ANYUNREAL_RHI_SHADER_COMMANDS_H
#define ANYUNREAL_RHI_SHADER_COMMANDS_H

#include "AnyUnreal/RHI/Shader/AgcShader.h"
#include "AnyUnreal/Shader/UserData.h"
#include "SceTypes.hpp"

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API void WriteShaderRegisters(CommandBuffer& commands, const AgcShader& shader, const Shader::UserDataLayout& layout, const Shader::UserDataInputs& inputs);
}

#endif
