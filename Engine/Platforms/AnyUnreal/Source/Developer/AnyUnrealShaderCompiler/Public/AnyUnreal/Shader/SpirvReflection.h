#ifndef ANYUNREAL_SHADER_SPIRV_REFLECTION_H
#define ANYUNREAL_SHADER_SPIRV_REFLECTION_H

#include "spirv_reflect.h"
#include <cstdint>
#include <span>

namespace AnyUnreal::Shader
{
    class ANYUNREALSHADERCOMPILER_API SpirvReflection final
    {
    public:
        explicit SpirvReflection(std::span<const std::uint32_t> words);
        ~SpirvReflection();
        SpirvReflection(const SpirvReflection&) = delete;
        SpirvReflection& operator=(const SpirvReflection&) = delete;
        const SpvReflectShaderModule* Get() const;
        void RemapBinding(const SpvReflectDescriptorBinding* descriptor, std::uint32_t binding);
        static void Check(SpvReflectResult result);

    private:
        SpvReflectShaderModule module{};
    };
}

#endif
