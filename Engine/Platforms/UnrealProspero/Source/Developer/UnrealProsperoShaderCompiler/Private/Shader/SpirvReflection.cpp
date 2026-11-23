#include "UnrealProspero/Shader/SpirvReflection.h"
#include <stdexcept>
#include <string>

namespace UnrealProspero::Shader
{
    void SpirvReflection::Check(SpvReflectResult result)
    {
        if (result != SPV_REFLECT_RESULT_SUCCESS) throw std::runtime_error("SPIR-V reflection failed: " + std::to_string(result));
    }

    SpirvReflection::SpirvReflection(std::span<const std::uint32_t> words)
    {
        if (words.size() < 5 || words.front() != 0x07230203) throw std::invalid_argument("Invalid SPIR-V header");
        Check(spvReflectCreateShaderModule(words.size_bytes(), words.data(), &module));
    }

    SpirvReflection::~SpirvReflection() { spvReflectDestroyShaderModule(&module); }
    const SpvReflectShaderModule* SpirvReflection::Get() const { return &module; }
    void SpirvReflection::RemapInputLocation(const SpvReflectInterfaceVariable* input, std::uint32_t location)
    {
        bool found = false;
        for (std::uint32_t index = 0; index < module.input_variable_count; ++index) found |= input == module.input_variables[index];
        if (!found || location >= 32) throw std::invalid_argument("Invalid vertex input location remapping");
        Check(spvReflectChangeInputVariableLocation(&module, input, location));
    }

    void SpirvReflection::RemapBinding(const SpvReflectDescriptorBinding* descriptor, std::uint32_t binding)
    {
        bool found = false;
        for (std::uint32_t index = 0; index < module.descriptor_binding_count; ++index) found |= descriptor == &module.descriptor_bindings[index];
        if (!found || binding == static_cast<std::uint32_t>(SPV_REFLECT_BINDING_NUMBER_DONT_CHANGE)) throw std::invalid_argument("Invalid descriptor remapping");
        Check(spvReflectChangeDescriptorBindingNumbers(&module, descriptor, binding, SPV_REFLECT_SET_NUMBER_DONT_CHANGE));
    }
}
