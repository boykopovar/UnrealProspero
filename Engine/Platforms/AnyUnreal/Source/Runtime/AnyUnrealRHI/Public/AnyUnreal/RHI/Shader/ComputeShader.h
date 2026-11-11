#ifndef ANYUNREAL_RHI_COMPUTE_SHADER_H
#define ANYUNREAL_RHI_COMPUTE_SHADER_H

#include "AnyUnreal/RHI/Shader/AgcShader.h"
#include "AnyUnreal/RHI/Shader/ShaderCode.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API ComputeShader final : public FRHIComputeShader
    {
    public:
        ComputeShader(TConstArrayView<uint8> source, const FShaderHash& hash, Shader::AgcTarget target);
        ~ComputeShader() noexcept override;
        const AgcShader& GetNativeShader() const;
        const ShaderCode& GetCode() const;

    private:
        ShaderCode code;
        AgcShader shader;
    };
}

#endif
