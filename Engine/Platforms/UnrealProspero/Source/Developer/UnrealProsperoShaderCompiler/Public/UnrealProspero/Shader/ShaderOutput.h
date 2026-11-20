#ifndef UNREALPROSPERO_SHADER_SHADER_OUTPUT_H
#define UNREALPROSPERO_SHADER_SHADER_OUTPUT_H

#include "UnrealProspero/Shader/AgcCompiler.h"
#include "UnrealProspero/Shader/SpirvCompiler.h"

namespace UnrealProspero::Shader
{
    UNREALPROSPEROSHADERCOMPILER_API std::vector<RuntimeBinding> BuildRuntimeBindings(const SpirvCompilation& spirv);
    UNREALPROSPEROSHADERCOMPILER_API std::vector<std::uint8_t> EncodeCompiledShader(const SpirvCompilation& spirv, const AgcCompilation& agc, ShaderStage stage, AgcTarget target, const ShaderIdentity& vertexIdentity = {});
}

#endif
