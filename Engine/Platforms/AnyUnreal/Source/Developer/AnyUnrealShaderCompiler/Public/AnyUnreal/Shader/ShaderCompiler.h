#ifndef ANYUNREAL_SHADER_COMPILER_H
#define ANYUNREAL_SHADER_COMPILER_H

#include "AnyUnreal/Shader/AgcCompiler.h"

struct FShaderCompilerInput;
struct FShaderCompilerOutput;
struct FShaderPreprocessOutput;

namespace AnyUnreal::Shader
{
    ANYUNREALSHADERCOMPILER_API void CompileShader(const FShaderCompilerInput& input, const FShaderPreprocessOutput& preprocessed, FShaderCompilerOutput& output, AgcTarget target, const FString& executable, const FString& workingRoot);
}

#endif
