#ifndef UNREALPROSPERO_SHADER_HLSL_COMPILER_H
#define UNREALPROSPERO_SHADER_HLSL_COMPILER_H

#include "CoreTypes.h"
#include <cstdint>
#include <string_view>
#include <vector>

struct FShaderCompilerInput;
struct FShaderCompilerOutput;

namespace UnrealProspero::Shader
{
    UNREALPROSPEROSHADERCOMPILER_API std::vector<std::uint32_t> CompileHlslToSpirv(std::string_view source, const FShaderCompilerInput& input, FShaderCompilerOutput& output);
}

#endif
