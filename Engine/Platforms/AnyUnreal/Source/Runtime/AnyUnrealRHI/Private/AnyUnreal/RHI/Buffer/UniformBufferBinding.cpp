#include "AnyUnreal/RHI/Buffer/UniformBufferBinding.h"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    UniformBufferBinding::UniformBufferBinding(const ShaderCode& shader, std::uint32_t parameterIndex, const UniformBuffer& buffer) : parameterIndex(parameterIndex), storage(buffer.Capture())
    {
        const auto& hashes = shader.GetResourceTable().ResourceTableLayoutHashes;
        if (parameterIndex >= static_cast<unsigned>(hashes.Num())) throw std::invalid_argument("Uniform buffer index exceeds the shader layout table");
        const auto expectedHash = hashes[parameterIndex];
        if (expectedHash != 0 && expectedHash != buffer.GetLayout().GetHash()) throw std::invalid_argument("Uniform buffer layout does not match the shader");
        for (const auto& binding : shader.GetBinary().Bindings)
        {
            if (binding.Kind != Shader::ResourceKind::UniformBuffer || binding.ParameterIndex != parameterIndex) continue;
            if (binding.Count != 1) throw std::runtime_error("Uniform buffer descriptor arrays are not implemented");
            const auto* constants = storage->GetConstants();
            if (!constants || constants->Size() < binding.BlockSize) throw std::invalid_argument("Uniform buffer constants do not cover the shader block");
            hasConstants = true;
        }
        for (const auto& binding : shader.GetResourceBindings())
        {
            if (binding.UniformBufferIndex != parameterIndex) continue;
            const auto& table = storage->GetResources();
            if (binding.ResourceIndex >= static_cast<unsigned>(table.Num()) || !table[binding.ResourceIndex]) throw std::invalid_argument("Uniform buffer shader resource is missing");
            resources.push_back({binding.Type, binding.ParameterIndex, table[binding.ResourceIndex]});
        }
        if (!hasConstants && resources.empty()) throw std::invalid_argument("Uniform buffer is unused by this shader");
    }

    const BufferAllocation* UniformBufferBinding::GetConstants() const
    {
        return hasConstants ? storage->GetConstants() : nullptr;
    }

    void UniformBufferBinding::ApplyConstants(DescriptorTables& descriptors) const
    {
        if (!hasConstants) return;
        const auto& constants = *storage->GetConstants();
        descriptors.SetBuffer(Shader::ResourceKind::UniformBuffer, parameterIndex, constants.Data(), constants.Size());
    }

    const std::vector<BoundUniformResource>& UniformBufferBinding::GetResources() const { return resources; }
}
