#include "AnyUnreal/Shader/SpirvReflection.h"
#include <stdexcept>
#include <string>

namespace AnyUnreal::Shader
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
}
