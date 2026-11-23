#ifndef UNREALPROSPERO_SHADER_SPIRV_REFLECTION_H
#define UNREALPROSPERO_SHADER_SPIRV_REFLECTION_H

#include "CoreTypes.h"
#include "spirv_reflect.h"
#include <cstdint>
#include <span>

namespace UnrealProspero::Shader
{
    class UNREALPROSPEROSHADERCOMPILER_API SpirvReflection final
    {
    public:
        explicit SpirvReflection(std::span<const std::uint32_t> words);
        ~SpirvReflection();
        SpirvReflection(const SpirvReflection&) = delete;
        SpirvReflection& operator=(const SpirvReflection&) = delete;
        const SpvReflectShaderModule* Get() const;
        void RemapInputLocation(const SpvReflectInterfaceVariable* input, std::uint32_t location);
        void RemapBinding(const SpvReflectDescriptorBinding* descriptor, std::uint32_t binding);
        static void Check(SpvReflectResult result);

    private:
        SpvReflectShaderModule module{};
    };
}

#endif
