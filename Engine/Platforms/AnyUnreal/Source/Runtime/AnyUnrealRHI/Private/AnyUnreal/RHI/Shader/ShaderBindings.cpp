#include "AnyUnreal/RHI/Shader/ShaderBindings.h"
#include "AnyUnreal/RHI/Buffer/BufferView.h"
#include "AnyUnreal/RHI/State/SamplerState.h"
#include "AnyUnreal/RHI/Texture/TextureResource.h"
#include "AnyUnreal/RHI/Texture/TextureView.h"
#include <stdexcept>
#include <utility>

namespace AnyUnreal::RHI
{
    void ValidateShaderResourceParameter(FRHIShaderParameterResource::EType type, FRHIResource* resource)
    {
        if (!resource) throw std::invalid_argument("Shader resource parameter is null");
        using Type = FRHIShaderParameterResource::EType;
        switch (type)
        {
        case Type::Texture:
            if (!EnumHasAnyFlags(ResolveTextureResource(resource).GetDesc().Flags, ETextureCreateFlags::ShaderResource)) throw std::invalid_argument("Texture was not created for shader access");
            break;
        case Type::Sampler:
            if (resource->GetType() != RRT_SamplerState) throw std::invalid_argument("Sampler parameter has the wrong resource type");
            break;
        case Type::ResourceView:
            if (resource->GetType() != RRT_ShaderResourceView) throw std::invalid_argument("Shader SRV parameter has the wrong resource type");
            break;
        case Type::UnorderedAccessView:
            if (resource->GetType() != RRT_UnorderedAccessView) throw std::invalid_argument("Shader UAV parameter has the wrong resource type");
            break;
        case Type::UniformBuffer:
            if (resource->GetType() != RRT_UniformBuffer) throw std::invalid_argument("Uniform buffer parameter has the wrong resource type");
            break;
        default:
            throw std::runtime_error("Shader resource parameter type is not implemented");
        }
    }

    ShaderBindings::ShaderBindings(FRHIShader& shader, const ShaderCode& code) : shader(&shader), code(code), descriptors(code.GetBinary().Bindings, code.GetUserData().GetAddressHigh()), constants(code.GetBinary().Bindings) {}
    ShaderBindings::ShaderBindings(ComputeShader& shader) : ShaderBindings(shader, shader.GetCode()) {}
    ShaderBindings::ShaderBindings(VertexShader& shader) : ShaderBindings(shader, shader.GetCode()) {}
    ShaderBindings::ShaderBindings(PixelShader& shader) : ShaderBindings(shader, shader.GetCode()) {}
    ShaderBindings::ShaderBindings(GeometryShader& shader) : ShaderBindings(shader, shader.GetCode()) {}

    const FRHIShader& ShaderBindings::GetShader() const { return *shader; }

    std::vector<GpuMemoryRange> ShaderBindings::GetMemoryRanges() const
    {
        if (!finalized || failed) throw std::runtime_error("Shader bindings are not finalized");
        auto result = descriptors.GetMemoryRanges();
        std::set<const BufferAllocation*> allocations;
        for (const auto& allocation : bufferAllocations) if (allocation) allocations.insert(allocation.get());
        for (const auto& entry : constantBuffers) allocations.insert(entry.second.get());
        for (const auto& uniform : uniforms)
        {
            if (const auto* allocation = uniform.GetConstants()) allocations.insert(allocation);
        }
        for (const auto& resource : resources)
        {
            const BufferAllocation* allocation = nullptr;
            switch (resource->GetType())
            {
            case RRT_Texture:
                allocation = &static_cast<TextureResource*>(resource.GetReference())->GetStorage();
                break;
            case RRT_ShaderResourceView:
                allocation = static_cast<FRHIShaderResourceView*>(resource.GetReference())->IsBuffer() ? nullptr : static_cast<TextureShaderResourceView*>(resource.GetReference())->GetStorage();
                break;
            case RRT_UnorderedAccessView:
                allocation = static_cast<FRHIUnorderedAccessView*>(resource.GetReference())->IsBuffer() ? nullptr : static_cast<TextureUnorderedAccessView*>(resource.GetReference())->GetStorage();
                break;
            case RRT_UniformBuffer:
            case RRT_SamplerState:
                break;
            default:
                throw std::runtime_error("Shader binding contains an unsupported resource");
            }
            if (allocation) allocations.insert(allocation);
        }
        for (const auto* allocation : allocations) result.push_back({allocation->Data(), allocation->Size()});
        return result;
    }

    void ShaderBindings::bindResource(FRHIShaderParameterResource::EType type, std::uint32_t index, FRHIResource* resource)
    {
        ValidateShaderResourceParameter(type, resource);
        using Type = FRHIShaderParameterResource::EType;
        switch (type)
        {
        case Type::Texture:
        {
            resource = &ResolveTextureResource(resource);
            const auto descriptor = static_cast<TextureResource*>(resource)->CreateViewDescriptor(false);
            descriptors.SetDescriptor(Shader::ResourceKind::ShaderResource, index, std::as_bytes(std::span(descriptor)));
            break;
        }
        case Type::Sampler:
        {
            const auto& descriptor = static_cast<SamplerState*>(resource)->GetDescriptor();
            descriptors.SetDescriptor(Shader::ResourceKind::Sampler, index, std::as_bytes(std::span(descriptor)));
            break;
        }
        case Type::ResourceView:
        {
            auto* view = static_cast<FRHIShaderResourceView*>(resource);
            if (view->IsBuffer())
            {
                const auto snapshot = static_cast<BufferShaderResourceView*>(view)->Capture();
                bufferAllocations.push_back(snapshot.CaptureStorage());
                const auto& descriptor = snapshot.GetDescriptor();
                descriptors.SetDescriptor(Shader::ResourceKind::ShaderResource, index, std::as_bytes(std::span(descriptor)));
            }
            else
            {
                const auto& descriptor = static_cast<TextureShaderResourceView*>(view)->GetDescriptor();
                descriptors.SetDescriptor(Shader::ResourceKind::ShaderResource, index, std::as_bytes(std::span(descriptor)));
            }
            break;
        }
        case Type::UnorderedAccessView:
        {
            auto* view = static_cast<FRHIUnorderedAccessView*>(resource);
            if (view->IsBuffer())
            {
                const auto snapshot = static_cast<BufferUnorderedAccessView*>(view)->Capture();
                bufferAllocations.push_back(snapshot.CaptureStorage());
                const auto& descriptor = snapshot.GetDescriptor();
                descriptors.SetDescriptor(Shader::ResourceKind::UnorderedAccess, index, std::as_bytes(std::span(descriptor)));
            }
            else
            {
                const auto& descriptor = static_cast<TextureUnorderedAccessView*>(view)->GetDescriptor();
                descriptors.SetDescriptor(Shader::ResourceKind::UnorderedAccess, index, std::as_bytes(std::span(descriptor)));
            }
            break;
        }
        case Type::UniformBuffer:
        {
            if (constants.Contains(index)) throw std::invalid_argument("Uniform buffer conflicts with loose shader constants");
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
                const auto source = std::as_bytes(std::span(data.GetData(), static_cast<std::size_t>(data.Num())));
                constants.Write(parameter.BufferIndex, parameter.BaseIndex, source, parameter.ByteOffset, parameter.ByteSize);
            }
        }
        catch (...)
        {
            failed = true;
            throw;
        }
    }

    void ShaderBindings::BindConstantData(const Shader::ConstantData& data)
    {
        if (finalized || failed) throw std::runtime_error("Shader bindings are no longer writable");
        try
        {
            for (const auto& [index, block] : data.GetBlocks())
            {
                if (block.Data.empty()) continue;
                if (uniformIndices.contains(index)) throw std::invalid_argument("Loose shader constants conflict with a uniform buffer");
                constants.Write(index, 0, block.Data, 0, block.Data.size());
            }
        }
        catch (...)
        {
            failed = true;
            throw;
        }
    }

    const DescriptorTables& ShaderBindings::FinalizeDescriptors()
    {
        if (failed) throw std::runtime_error("Shader bindings contain a failed update");
        if (!finalized)
        {
            try
            {
                for (const auto& [index, block] : constants.GetBlocks())
                {
                    if (block.Data.empty()) continue;
                    auto buffer = std::make_unique<BufferAllocation>(block.Data.size(), code.GetUserData().GetAddressHigh());
                    buffer->Upload(0, block.Data);
                    descriptors.SetBuffer(Shader::ResourceKind::UniformBuffer, index, buffer->Data(), buffer->Size());
                    constantBuffers[index] = std::move(buffer);
                }
                Shader::UserDataInputs inputs;
                descriptors.Apply(inputs);
                finalized = true;
            }
            catch (...)
            {
                failed = true;
                throw;
            }
        }
        return descriptors;
    }

    Shader::UserDataInputs ShaderBindings::Finalize(Shader::UserDataInputs inputs)
    {
        FinalizeDescriptors().Apply(inputs);
        code.GetUserData().Build(inputs);
        return inputs;
    }
}
