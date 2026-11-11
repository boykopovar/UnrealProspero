#ifndef ANYUNREAL_SHADER_PIPELINE_COMPILER_H
#define ANYUNREAL_SHADER_PIPELINE_COMPILER_H

#include "AnyUnreal/Shader/GeometryCompiler.h"

namespace AnyUnreal::Shader
{
    ANYUNREALSHADERCOMPILER_API void CompileShaderPipeline(std::span<const ShaderCompilationStage> stages, AgcTarget target, const FString& executable, const FString& workingRoot);
}

#endif
