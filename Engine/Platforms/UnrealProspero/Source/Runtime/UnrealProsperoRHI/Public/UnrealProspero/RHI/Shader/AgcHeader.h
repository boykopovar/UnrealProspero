#ifndef UNREALPROSPERO_RHI_SHADER_AGC_HEADER_H
#define UNREALPROSPERO_RHI_SHADER_AGC_HEADER_H

#include "UnrealProspero/Shader/ShaderBinary.h"

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API void ValidateAgcHeader(std::span<const std::uint8_t> header, std::span<const std::uint8_t> code, Shader::ShaderStage stage);
}

#endif
