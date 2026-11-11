#ifndef UNREALPROSPERO_SHADER_PIPELINE_COMPILER_H
#define UNREALPROSPERO_SHADER_PIPELINE_COMPILER_H

#include "UnrealProspero/Shader/GeometryCompiler.h"

namespace UnrealProspero::Shader
{
    UNREALPROSPEROSHADERCOMPILER_API void CompileShaderPipeline(std::span<const ShaderCompilationStage> stages, AgcTarget target, const FString& executable, const FString& workingRoot);
}

#endif
