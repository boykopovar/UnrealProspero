#include "UnrealProspero/RHI/Shader/ShaderParameters.h"
#include <stdexcept>

namespace UnrealProspero::RHI
{
    ShaderParameters::ShaderParameters(ShaderReference shader, const ShaderCode& code) : shader(std::move(shader)), constants(code.GetBinary().Bindings) {}
    ShaderParameters::ShaderParameters(ComputeShader& shader) : ShaderParameters(TRefCountPtr<ComputeShader>(&shader), shader.GetCode()) {}
    ShaderParameters::ShaderParameters(VertexShader& shader) : ShaderParameters(TRefCountPtr<VertexShader>(&shader), shader.GetCode()) {}
    ShaderParameters::ShaderParameters(PixelShader& shader) : ShaderParameters(TRefCountPtr<PixelShader>(&shader), shader.GetCode()) {}
    ShaderParameters::ShaderParameters(GeometryShader& shader) : ShaderParameters(TRefCountPtr<GeometryShader>(&shader), shader.GetCode()) {}

    const FRHIShader& ShaderParameters::GetShader() const
    {
        return std::visit([](const auto& value) -> const FRHIShader& { return *value; }, shader);
    }

    void ShaderParameters::SetConstants(TConstArrayView<uint8> data, TConstArrayView<FRHIShaderParameter> parameters)
    {
        if (failed) throw std::runtime_error("Shader parameters contain a failed update");
        try
        {
            if (data.Num() < 0 || (data.Num() != 0 && !data.GetData())) throw std::invalid_argument("Invalid shader parameter data");
            const auto source = std::as_bytes(std::span(data.GetData(), static_cast<std::size_t>(data.Num())));
            for (const auto& parameter : parameters)
            {
                if (resources.contains({FRHIShaderParameterResource::EType::UniformBuffer, parameter.BufferIndex})) throw std::invalid_argument("Loose shader constants conflict with a uniform buffer");
                constants.Write(parameter.BufferIndex, parameter.BaseIndex, source, parameter.ByteOffset, parameter.ByteSize);
            }
        }
        catch (...)
        {
            failed = true;
            throw;
        }
    }

    void ShaderParameters::SetResources(TConstArrayView<FRHIShaderParameterResource> parameters)
    {
        if (failed) throw std::runtime_error("Shader parameters contain a failed update");
        try
        {
            for (const auto& parameter : parameters)
            {
                ValidateShaderResourceParameter(parameter.Type, parameter.Resource);
                if (parameter.Type == FRHIShaderParameterResource::EType::UniformBuffer && constants.Contains(parameter.Index)) throw std::invalid_argument("Uniform buffer conflicts with loose shader constants");
                resources[{parameter.Type, parameter.Index}] = parameter.Resource;
            }
        }
        catch (...)
        {
            failed = true;
            throw;
        }
    }

    std::unique_ptr<ShaderBindings> ShaderParameters::CreateBindings() const
    {
        if (failed) throw std::runtime_error("Shader parameters contain a failed update");
        auto result = std::visit([](const auto& value) { return std::make_unique<ShaderBindings>(*value); }, shader);
        for (const auto& [key, resource] : resources)
        {
            const FRHIShaderParameterResource parameter(key.first, resource.GetReference(), key.second);
            result->Bind({&parameter, 1});
        }
        result->BindConstantData(constants);
        return result;
    }
}
