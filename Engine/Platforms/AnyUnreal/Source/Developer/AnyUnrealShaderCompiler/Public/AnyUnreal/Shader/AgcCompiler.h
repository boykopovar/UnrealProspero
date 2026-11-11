#ifndef ANYUNREAL_SHADER_AGC_COMPILER_H
#define ANYUNREAL_SHADER_AGC_COMPILER_H

#include "AnyUnreal/Shader/DescriptorLayout.h"
#include "Containers/UnrealString.h"
#include "RHIDefinitions.h"

namespace AnyUnreal::Shader
{
    struct AgcCompilation
    {
        std::vector<std::uint8_t> Header;
        std::vector<std::uint8_t> Code;
        std::vector<std::uint8_t> Metadata;
    };

    struct AgcTessellationCompilation
    {
        AgcCompilation Control;
        AgcCompilation Evaluation;
    };

    ANYUNREALSHADERCOMPILER_API AgcCompilation CompileAgc(std::span<const std::uint32_t> words, std::span<const DescriptorBinding> descriptors, EShaderFrequency stage, AgcTarget target, const FString& entryPoint, const FString& executable, const FString& workingRoot);
    ANYUNREALSHADERCOMPILER_API AgcCompilation CompileGeometryAgc(std::span<const std::uint32_t> vertexWords, std::span<const std::uint32_t> geometryWords, std::span<const DescriptorBinding> descriptors, AgcTarget target, const FString& vertexEntryPoint, const FString& geometryEntryPoint, const FString& executable, const FString& workingRoot);
    ANYUNREALSHADERCOMPILER_API AgcTessellationCompilation CompileTessellationAgc(std::span<const std::uint32_t> vertexWords, std::span<const std::uint32_t> controlWords, std::span<const std::uint32_t> evaluationWords, std::span<const DescriptorBinding> descriptors, AgcTarget target, std::uint32_t patchControlPoints, const FString& vertexEntryPoint, const FString& controlEntryPoint, const FString& evaluationEntryPoint, const FString& executable, const FString& workingRoot);
}

#endif
