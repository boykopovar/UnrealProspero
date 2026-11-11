#ifndef UNREALPROSPERO_SHADER_GEOMETRY_COMPILER_H
#define UNREALPROSPERO_SHADER_GEOMETRY_COMPILER_H

#include "UnrealProspero/Shader/ShaderCompiler.h"

namespace UnrealProspero::Shader
{
    struct ShaderCompilationStage
    {
        const FShaderCompilerInput& Input;
        const FShaderPreprocessOutput& Preprocessed;
        FShaderCompilerOutput& Output;
    };

    UNREALPROSPEROSHADERCOMPILER_API void CompileGeometryShaders(const ShaderCompilationStage& vertex, const ShaderCompilationStage& geometry, AgcTarget target, const FString& executable, const FString& workingRoot);
}

#endif
