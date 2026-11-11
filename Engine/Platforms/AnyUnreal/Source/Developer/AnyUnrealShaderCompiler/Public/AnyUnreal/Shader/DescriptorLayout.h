#ifndef ANYUNREAL_SHADER_DESCRIPTOR_LAYOUT_H
#define ANYUNREAL_SHADER_DESCRIPTOR_LAYOUT_H

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace AnyUnreal::Shader
{
    struct DescriptorBinding
    {
        std::uint32_t Set;
        std::uint32_t Binding;
        std::uint32_t Count;
        std::uint32_t Offset;
        std::uint32_t Stride;
        std::string Type;
        std::string Name;
    };

    ANYUNREALSHADERCOMPILER_API std::vector<DescriptorBinding> ReflectDescriptorLayout(std::span<const std::uint32_t> spirv, const std::string& entryPoint);
}

#endif
