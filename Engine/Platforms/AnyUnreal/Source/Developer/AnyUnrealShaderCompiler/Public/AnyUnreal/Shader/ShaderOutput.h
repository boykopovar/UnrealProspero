#ifndef ANYUNREAL_SHADER_SHADER_OUTPUT_H
#define ANYUNREAL_SHADER_SHADER_OUTPUT_H

#include "AnyUnreal/Shader/AgcCompiler.h"
#include "AnyUnreal/Shader/SpirvCompiler.h"

namespace AnyUnreal::Shader
{
    ANYUNREALSHADERCOMPILER_API std::vector<std::uint8_t> EncodeCompiledShader(const SpirvCompilation& spirv, const AgcCompilation& agc, ShaderStage stage, AgcTarget target);
}

#endif
