#ifndef ANYUNREAL_SHADER_SPIRV_COMPILER_H
#define ANYUNREAL_SHADER_SPIRV_COMPILER_H

#include "AnyUnreal/Shader/Parameters.h"
#include "AnyUnreal/Shader/VertexInputs.h"
#include "RHIResources.h"

struct FShaderPreprocessOutput;

namespace AnyUnreal::Shader
{
    struct SpirvCompilation
    {
        std::vector<std::uint32_t> Words;
        std::vector<DescriptorBinding> Descriptors;
        std::vector<ParameterBinding> Parameters;
        FShaderResourceTable ResourceTable;
        std::vector<VertexInput> VertexInputs;
    };

    ANYUNREALSHADERCOMPILER_API SpirvCompilation CompileSpirv(const FShaderCompilerInput& input, const FShaderPreprocessOutput& preprocessed, FShaderCompilerOutput& output);
}

#endif
