#include "AnyUnreal/RHI/Shader/ShaderBindings.h"
#include "AnyUnreal/RHI/Buffer/BufferView.h"
#include <stdexcept>
#include <algorithm>
#include <utility>

namespace AnyUnreal::RHI
{
    ShaderBindings::ShaderBindings(FRHIShader& shader, const ShaderCode& code) : shader(&shader), code(code), descriptors(code.GetBinary().Bindings, code.GetUserData().GetAddressHigh()) {}
    ShaderBindings::ShaderBindings(ComputeShader& shader) : ShaderBindings(shader, shader.GetCode()) {}
    ShaderBindings::ShaderBindings(VertexShader& shader) : ShaderBindings(shader, shader.GetCode()) {}
    ShaderBindings::ShaderBindings(PixelShader& shader) : ShaderBindings(shader, shader.GetCode()) {}

    const FRHIShader& ShaderBindings::GetShader() const { return *shader; }

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
            if (constantBuffers.contains(index)) throw std::invalid_argument("Uniform buffer conflicts with loose shader constants");
            uniformIndices.insert(index);
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

    void ShaderBindings::BindConstants(TConstArrayView<uint8> data, TConstArrayView<FRHIShaderParameter> parameters)
    {
        if (finalized || failed) throw std::runtime_error("Shader bindings are no longer writable");
        try
        {
            if (data.Num() < 0 || (data.Num() != 0 && !data.GetData())) throw std::invalid_argument("Invalid shader parameter data");
            for (const auto& parameter : parameters)
            {
                if (parameter.ByteSize == 0 || parameter.ByteOffset > data.Num() || parameter.ByteSize > data.Num() - parameter.ByteOffset) throw std::out_of_range("Shader parameter exceeds its source data");
                if (uniformIndices.contains(parameter.BufferIndex)) throw std::invalid_argument("Loose shader constants conflict with a uniform buffer");
                const auto& bindings = code.GetBinary().Bindings;
                const auto binding = std::find_if(bindings.begin(), bindings.end(), [&](const auto& item) { return item.Kind == Shader::ResourceKind::UniformBuffer && item.ParameterIndex == parameter.BufferIndex; });
                if (binding == bindings.end() || binding->Count != 1) throw std::invalid_argument("Shader constant buffer binding does not exist");
                if (parameter.BaseIndex > binding->BlockSize || parameter.ByteSize > binding->BlockSize - parameter.BaseIndex) throw std::out_of_range("Shader parameter exceeds its constant buffer");
                auto& buffer = constantBuffers[parameter.BufferIndex];
                if (!buffer) buffer = std::make_unique<BufferAllocation>(binding->BlockSize, code.GetUserData().GetAddressHigh());
                const auto* source = reinterpret_cast<const std::byte*>(data.GetData() + parameter.ByteOffset);
                buffer->Upload(parameter.BaseIndex, {source, parameter.ByteSize});
                descriptors.SetBuffer(Shader::ResourceKind::UniformBuffer, parameter.BufferIndex, buffer->Data(), buffer->Size());
            }
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
