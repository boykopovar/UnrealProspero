#ifndef UNREALPROSPERO_SHADER_PARAMETERS_H
#define UNREALPROSPERO_SHADER_PARAMETERS_H

#include "UnrealProspero/Shader/DescriptorLayout.h"
#include <cstddef>

struct FShaderCompilerInput;
struct FShaderCompilerOutput;
class FShaderParameterParser;

namespace UnrealProspero::Shader
{
    struct ParameterBinding
    {
        std::size_t DescriptorIndex;
        std::uint16_t ParameterIndex;
    };

    UNREALPROSPEROSHADERCOMPILER_API std::vector<ParameterBinding> RegisterParameters(std::span<const DescriptorBinding> descriptors, const FShaderCompilerInput& input, const FShaderParameterParser& parser, FShaderCompilerOutput& output);
}

#endif
