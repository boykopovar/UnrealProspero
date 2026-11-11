#ifndef UNREALPROSPERO_SHADER_COMPILER_H
#define UNREALPROSPERO_SHADER_COMPILER_H

#include "UnrealProspero/Shader/AgcCompiler.h"

struct FShaderCompilerInput;
struct FShaderCompilerOutput;
struct FShaderPreprocessOutput;

namespace UnrealProspero::Shader
{
    UNREALPROSPEROSHADERCOMPILER_API void CompileShader(const FShaderCompilerInput& input, const FShaderPreprocessOutput& preprocessed, FShaderCompilerOutput& output, AgcTarget target, const FString& executable, const FString& workingRoot);
}

#endif
