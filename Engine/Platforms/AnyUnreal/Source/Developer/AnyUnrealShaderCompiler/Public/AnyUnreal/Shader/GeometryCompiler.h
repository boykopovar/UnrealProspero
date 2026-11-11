#ifndef ANYUNREAL_SHADER_GEOMETRY_COMPILER_H
#define ANYUNREAL_SHADER_GEOMETRY_COMPILER_H

#include "AnyUnreal/Shader/ShaderCompiler.h"

namespace AnyUnreal::Shader
{
    struct ShaderCompilationStage
    {
        const FShaderCompilerInput& Input;
        const FShaderPreprocessOutput& Preprocessed;
        FShaderCompilerOutput& Output;
    };

    ANYUNREALSHADERCOMPILER_API void CompileGeometryShaders(const ShaderCompilationStage& vertex, const ShaderCompilationStage& geometry, AgcTarget target, const FString& executable, const FString& workingRoot);
}

#endif
