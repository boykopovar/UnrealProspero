#ifndef UNREALPROSPERO_SHADER_SPIRV_COMPILER_H
#define UNREALPROSPERO_SHADER_SPIRV_COMPILER_H

#include "UnrealProspero/Shader/Parameters.h"
#include "UnrealProspero/Shader/VertexInputs.h"
#include "RHIResources.h"

class FShaderPreprocessOutput;

namespace UnrealProspero::Shader
{
    struct SpirvCompilation
    {
        std::vector<std::uint32_t> Words;
        std::vector<DescriptorBinding> Descriptors;
        std::vector<ParameterBinding> Parameters;
        FShaderResourceTable ResourceTable;
        std::vector<VertexInput> VertexInputs;
    };

    UNREALPROSPEROSHADERCOMPILER_API SpirvCompilation CompileSpirv(const FShaderCompilerInput& input, const FShaderPreprocessOutput& preprocessed, FShaderCompilerOutput& output);
}

#endif
