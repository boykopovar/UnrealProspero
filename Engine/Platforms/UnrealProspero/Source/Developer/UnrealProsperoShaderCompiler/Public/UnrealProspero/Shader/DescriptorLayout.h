#ifndef UNREALPROSPERO_SHADER_DESCRIPTOR_LAYOUT_H
#define UNREALPROSPERO_SHADER_DESCRIPTOR_LAYOUT_H

#include "UnrealProspero/Shader/ShaderBinary.h"
#include <span>
#include <string>
#include <vector>

namespace UnrealProspero::Shader
{
    struct ConstantMember
    {
        std::string Name;
        std::uint32_t Offset;
        std::uint32_t Size;
        bool operator==(const ConstantMember&) const = default;
    };

    struct DescriptorBinding
    {
        std::uint32_t Set;
        std::uint32_t Binding;
        std::uint32_t Count;
        std::uint32_t Offset;
        std::uint32_t Stride;
        std::string Type;
        std::string Name;
        ResourceKind Kind;
        std::uint32_t BlockSize;
        std::vector<ConstantMember> Members;
        bool operator==(const DescriptorBinding&) const = default;
    };

    UNREALPROSPEROSHADERCOMPILER_API std::vector<DescriptorBinding> ReflectDescriptorLayout(std::span<const std::uint32_t> spirv, const std::string& entryPoint);
}

#endif
