#ifndef UNREALPROSPERO_RHI_SHADER_PARAMETERS_H
#define UNREALPROSPERO_RHI_SHADER_PARAMETERS_H

#include "UnrealProspero/RHI/Shader/ShaderBindings.h"
#include <utility>
#include <variant>

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API ShaderParameters final
    {
    public:
        explicit ShaderParameters(ComputeShader& shader);
        explicit ShaderParameters(VertexShader& shader);
        explicit ShaderParameters(PixelShader& shader);
        explicit ShaderParameters(GeometryShader& shader);
        const FRHIShader& GetShader() const;
        void SetConstants(TConstArrayView<uint8> data, TConstArrayView<FRHIShaderParameter> parameters);
        void SetResources(TConstArrayView<FRHIShaderParameterResource> parameters);
        std::unique_ptr<ShaderBindings> CreateBindings() const;

    private:
        using ShaderReference = std::variant<TRefCountPtr<ComputeShader>, TRefCountPtr<VertexShader>, TRefCountPtr<PixelShader>, TRefCountPtr<GeometryShader>>;
        ShaderParameters(ShaderReference shader, const ShaderCode& code);
        ShaderReference shader;
        Shader::ConstantData constants;
        std::map<std::pair<FRHIShaderParameterResource::EType, std::uint16_t>, TRefCountPtr<FRHIResource>> resources;
        bool failed = false;
    };
}

#endif
