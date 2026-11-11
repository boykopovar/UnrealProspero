#ifndef ANYUNREAL_RHI_SHADER_AGC_HEADER_H
#define ANYUNREAL_RHI_SHADER_AGC_HEADER_H

#include "AnyUnreal/Shader/ShaderBinary.h"

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API void ValidateAgcHeader(std::span<const std::uint8_t> header, std::span<const std::uint8_t> code, Shader::ShaderStage stage);
}

#endif
