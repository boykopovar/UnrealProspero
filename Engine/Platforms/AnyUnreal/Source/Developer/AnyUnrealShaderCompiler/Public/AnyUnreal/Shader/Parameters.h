#ifndef ANYUNREAL_SHADER_PARAMETERS_H
#define ANYUNREAL_SHADER_PARAMETERS_H

#include "AnyUnreal/Shader/DescriptorLayout.h"
#include <cstddef>

struct FShaderCompilerInput;
struct FShaderCompilerOutput;
class FShaderParameterParser;

namespace AnyUnreal::Shader
{
    struct ParameterBinding
    {
        std::size_t DescriptorIndex;
        std::uint16_t ParameterIndex;
    };

    ANYUNREALSHADERCOMPILER_API std::vector<ParameterBinding> RegisterParameters(std::span<const DescriptorBinding> descriptors, const FShaderCompilerInput& input, const FShaderParameterParser& parser, FShaderCompilerOutput& output);
}

#endif
