#ifndef ANYUNREAL_SHADER_HLSL_COMPILER_H
#define ANYUNREAL_SHADER_HLSL_COMPILER_H

#include <cstdint>
#include <string_view>
#include <vector>

struct FShaderCompilerInput;
struct FShaderCompilerOutput;

namespace AnyUnreal::Shader
{
    ANYUNREALSHADERCOMPILER_API std::vector<std::uint32_t> CompileHlslToSpirv(std::string_view source, const FShaderCompilerInput& input, FShaderCompilerOutput& output);
}

#endif
