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

    ANYUNREALSHADERCOMPILER_API AgcCompilation CompileAgc(std::span<const std::uint32_t> words, std::span<const DescriptorBinding> descriptors, EShaderFrequency stage, AgcTarget target, const FString& entryPoint, const FString& executable, const FString& workingRoot);
}

#endif
