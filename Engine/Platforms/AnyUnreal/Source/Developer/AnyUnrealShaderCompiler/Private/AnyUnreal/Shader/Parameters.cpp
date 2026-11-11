#include "AnyUnreal/Shader/Parameters.h"
#include "ShaderCompilerCommon.h"
#include "ShaderParameterParser.h"
#include <array>
#include <limits>
#include <set>
#include <stdexcept>

namespace AnyUnreal::Shader
{
    std::vector<ParameterBinding> RegisterParameters(std::span<const DescriptorBinding> descriptors, const FShaderCompilerInput& input, const FShaderParameterParser& parser, FShaderCompilerOutput& output)
    {
        static_assert(FShaderParametersMetadata::kRootCBufferBindingIndex == 0);
        const std::string globalsName = CrossCompiler::FShaderConductorContext::GetIdentifierTable().GlobalsUniformBuffer;
        const std::string rootName = TCHAR_TO_UTF8(FShaderParametersMetadata::kRootUniformBufferBindingName);
        std::array<std::uint32_t, 4> counts{};
        bool hasConstants = false;
        std::set<std::string> descriptorNames;
        for (const auto& descriptor : descriptors)
        {
            if (descriptor.Name.empty() || descriptor.Name.find('\0') != std::string::npos) throw std::invalid_argument("Shader parameter has no valid name");
            if (!descriptorNames.insert(descriptor.Name).second) throw std::invalid_argument("Duplicate shader parameter name");
            if (static_cast<std::size_t>(descriptor.Kind) >= counts.size() || descriptor.Type == "input-attachment") throw std::invalid_argument("Unsupported Unreal shader resource category");
            if (descriptor.Count == 0 || descriptor.Count > std::numeric_limits<std::uint16_t>::max()) throw std::overflow_error("Shader resource count exceeds Unreal parameter limits");
            if (descriptor.Kind != ResourceKind::UniformBuffer) continue;
            if (descriptor.Count != 1) throw std::invalid_argument("Uniform buffer arrays require separate Unreal parameter bindings");
            if (descriptor.BlockSize == 0 || descriptor.BlockSize > std::numeric_limits<std::uint16_t>::max()) throw std::overflow_error("Uniform buffer exceeds Unreal parameter limits");
            for (const auto& member : descriptor.Members)
            {
                if (member.Name.empty() || member.Name.find('\0') != std::string::npos || member.Size == 0 || static_cast<std::uint64_t>(member.Offset) + member.Size > descriptor.BlockSize) throw std::invalid_argument("Invalid reflected uniform buffer member");
            }
            if (descriptor.Name == globalsName || descriptor.Name == rootName)
            {
                if (hasConstants) throw std::invalid_argument("Global and root constants cannot share the Unreal root buffer slot");
                hasConstants = true;
            }
        }
        counts[static_cast<std::size_t>(ResourceKind::UniformBuffer)] = hasConstants ? 1 : 0;
        std::vector<ParameterBinding> result;
        result.reserve(descriptors.size());
        for (std::size_t index = 0; index < descriptors.size(); ++index)
        {
            const auto& descriptor = descriptors[index];
            auto& next = counts[static_cast<std::size_t>(descriptor.Kind)];
            const bool constants = descriptor.Kind == ResourceKind::UniformBuffer && (descriptor.Name == globalsName || descriptor.Name == rootName);
            const auto slot = constants ? 0 : next;
            if (!constants)
            {
                if (static_cast<std::uint64_t>(next) + descriptor.Count > std::numeric_limits<std::uint16_t>::max()) throw std::overflow_error("Shader resource slots exceed Unreal parameter limits");
                next += descriptor.Count;
            }
            result.push_back({index, static_cast<std::uint16_t>(slot)});
        }
        for (const auto& binding : result)
        {
            const auto& descriptor = descriptors[binding.DescriptorIndex];
            const FString name(UTF8_TO_TCHAR(descriptor.Name.c_str()));
            const int32 slot = binding.ParameterIndex;
            const int32 count = static_cast<int32>(descriptor.Count);
            switch (descriptor.Kind)
            {
                case ResourceKind::Sampler: HandleReflectedShaderSampler(name, slot, count, output); break;
                case ResourceKind::ShaderResource: HandleReflectedShaderResource(name, slot, count, output); break;
                case ResourceKind::UnorderedAccess: HandleReflectedShaderUAV(name, slot, count, output); break;
                case ResourceKind::UniformBuffer:
                {
                    if (descriptor.Name == rootName)
                    {
                        HandleReflectedRootConstantBuffer(static_cast<int32>(descriptor.BlockSize), output);
                        for (const auto& member : descriptor.Members) HandleReflectedRootConstantBufferMember(input, parser, FString(UTF8_TO_TCHAR(member.Name.c_str())), member.Offset, member.Size, output);
                    }
                    else if (descriptor.Name == globalsName)
                    {
                        for (const auto& member : descriptor.Members) HandleReflectedGlobalConstantBufferMember(input, parser, FString(UTF8_TO_TCHAR(member.Name.c_str())), slot, member.Offset, member.Size, output);
                    }
                    else
                    {
                        HandleReflectedUniformBuffer(input, name, slot, static_cast<int32>(descriptor.BlockSize), output);
                        const auto reason = ShouldReflectUniformBufferMembers(input, name);
                        for (const auto& member : descriptor.Members) HandleReflectedUniformBufferConstantBufferMember(input, reason, name, slot, FString(UTF8_TO_TCHAR(member.Name.c_str())), member.Offset, member.Size, output);
                    }
                    break;
                }
                default: throw std::invalid_argument("Unsupported Unreal shader resource category");
            }
        }
        return result;
    }
}
