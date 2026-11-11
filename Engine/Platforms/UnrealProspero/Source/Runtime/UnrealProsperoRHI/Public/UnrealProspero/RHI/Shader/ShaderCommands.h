#ifndef UNREALPROSPERO_RHI_SHADER_COMMANDS_H
#define UNREALPROSPERO_RHI_SHADER_COMMANDS_H

#include "UnrealProspero/RHI/Shader/AgcShader.h"
#include "UnrealProspero/Shader/UserData.h"
#include "SceTypes.hpp"

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API void WriteShaderRegisters(CommandBuffer& commands, const AgcShader& shader, const Shader::UserDataLayout& layout, const Shader::UserDataInputs& inputs);
}

#endif
