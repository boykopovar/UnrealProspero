#ifndef ANYUNREAL_RHI_COMPUTE_PARAMETERS_H
#define ANYUNREAL_RHI_COMPUTE_PARAMETERS_H

#include "AnyUnreal/RHI/Shader/ShaderBindings.h"
#include <utility>

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API ComputeParameters final
    {
    public:
        explicit ComputeParameters(ComputeShader& shader);
        void SetConstants(TConstArrayView<uint8> data, TConstArrayView<FRHIShaderParameter> parameters);
        void SetResources(TConstArrayView<FRHIShaderParameterResource> parameters);
        std::unique_ptr<ShaderBindings> CreateBindings() const;

    private:
        TRefCountPtr<ComputeShader> shader;
        Shader::ConstantData constants;
        std::map<std::pair<FRHIShaderParameterResource::EType, std::uint16_t>, TRefCountPtr<FRHIResource>> resources;
        bool failed = false;
    };
}

#endif
