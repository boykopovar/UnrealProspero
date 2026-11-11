#include "AnyUnreal/RHI/Shader/ShaderBindings.h"
#include "AnyUnreal/RHI/Buffer/BufferView.h"
#include <stdexcept>
#include <utility>

namespace AnyUnreal::RHI
{
    ShaderBindings::ShaderBindings(FRHIShader& shader, const ShaderCode& code) : shader(&shader), code(code), descriptors(code.GetBinary().Bindings, code.GetUserData().GetAddressHigh()) {}
    ShaderBindings::ShaderBindings(ComputeShader& shader) : ShaderBindings(shader, shader.GetCode()) {}
    ShaderBindings::ShaderBindings(VertexShader& shader) : ShaderBindings(shader, shader.GetCode()) {}
    ShaderBindings::ShaderBindings(PixelShader& shader) : ShaderBindings(shader, shader.GetCode()) {}

    void ShaderBindings::bindResource(FRHIShaderParameterResource::EType type, std::uint32_t index, FRHIResource* resource)
    {
        if (!resource) throw std::invalid_argument("Shader resource parameter is null");
        using Type = FRHIShaderParameterResource::EType;
        switch (type)
        {
        case Type::ResourceView:
        {
            if (resource->GetType() != RRT_ShaderResourceView) throw std::invalid_argument("Shader SRV parameter has the wrong resource type");
            auto* view = static_cast<FRHIShaderResourceView*>(resource);
            if (!view->IsBuffer()) throw std::runtime_error("Texture resource bindings are not implemented");
            const auto& descriptor = static_cast<BufferShaderResourceView*>(view)->GetDescriptor();
            descriptors.SetDescriptor(Shader::ResourceKind::ShaderResource, index, std::as_bytes(std::span(descriptor)));
            break;
        }
        case Type::UnorderedAccessView:
        {
            if (resource->GetType() != RRT_UnorderedAccessView) throw std::invalid_argument("Shader UAV parameter has the wrong resource type");
            auto* view = static_cast<FRHIUnorderedAccessView*>(resource);
            if (!view->IsBuffer()) throw std::runtime_error("Texture unordered access bindings are not implemented");
            const auto& descriptor = static_cast<BufferUnorderedAccessView*>(view)->GetDescriptor();
            descriptors.SetDescriptor(Shader::ResourceKind::UnorderedAccess, index, std::as_bytes(std::span(descriptor)));
            break;
        }
        case Type::UniformBuffer:
        {
            if (resource->GetType() != RRT_UniformBuffer) throw std::invalid_argument("Uniform buffer parameter has the wrong resource type");
            UniformBufferBinding binding(code, index, *static_cast<UniformBuffer*>(resource));
            binding.ApplyConstants(descriptors);
            for (const auto& member : binding.GetResources()) bindResource(member.Type, member.ParameterIndex, member.Resource.GetReference());
            uniforms.push_back(std::move(binding));
            break;
        }
        default:
            throw std::runtime_error("Shader resource parameter type is not implemented");
        }
        resources.emplace_back(resource);
    }

    void ShaderBindings::Bind(TConstArrayView<FRHIShaderParameterResource> parameters)
    {
        if (finalized || failed) throw std::runtime_error("Shader bindings are no longer writable");
        try
        {
            for (const auto& parameter : parameters) bindResource(parameter.Type, parameter.Index, parameter.Resource);
        }
        catch (...)
        {
            failed = true;
            throw;
        }
    }

    Shader::UserDataInputs ShaderBindings::Finalize(Shader::UserDataInputs inputs)
    {
        if (failed) throw std::runtime_error("Shader bindings contain a failed update");
        descriptors.Apply(inputs);
        code.GetUserData().Build(inputs);
        finalized = true;
        return inputs;
    }
}
