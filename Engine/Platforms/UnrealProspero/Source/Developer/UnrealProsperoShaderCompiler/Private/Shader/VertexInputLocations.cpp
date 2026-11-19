#include "UnrealProspero/Shader/VertexInputLocations.h"
#include "UnrealProspero/Shader/SpirvReflection.h"
#include "UnrealProspero/Shader/VertexInputs.h"
#include <charconv>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace UnrealProspero::Shader
{
    std::vector<std::uint32_t> NormalizeVertexInputLocations(std::span<const std::uint32_t> words, const std::string& entryPoint)
    {
        if (entryPoint.empty() || entryPoint.find('\0') != std::string::npos) throw std::invalid_argument("Invalid vertex shader entry point");
        SpirvReflection reflection(words);
        const auto* entry = spvReflectGetEntryPoint(reflection.Get(), entryPoint.c_str());
        if (!entry || entry->shader_stage != SPV_REFLECT_SHADER_STAGE_VERTEX_BIT) throw std::invalid_argument("Input location normalization requires a vertex shader");
        constexpr std::string_view prefix = "in.var.ATTRIBUTE";
        std::vector<std::pair<const SpvReflectInterfaceVariable*, std::uint32_t>> remaps;
        for (std::uint32_t index = 0; index < entry->input_variable_count; ++index)
        {
            const auto* input = entry->input_variables[index];
            if ((input->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) || !input->name) continue;
            const std::string_view name(input->name);
            if (!name.starts_with(prefix)) continue;
            const auto suffix = name.substr(prefix.size());
            std::uint32_t location = 0;
            const auto parsed = std::from_chars(suffix.data(), suffix.data() + suffix.size(), location);
            if (parsed.ec != std::errc{} || parsed.ptr != suffix.data() + suffix.size() || location >= 32) throw std::invalid_argument("Invalid vertex ATTRIBUTE semantic index");
            remaps.emplace_back(input, location);
        }
        for (const auto& [input, location] : remaps) reflection.RemapInputLocation(input, location);
        const auto size = spvReflectGetCodeSize(reflection.Get());
        if (size % sizeof(std::uint32_t) != 0) throw std::runtime_error("Invalid vertex SPIR-V size after location normalization");
        const auto* code = spvReflectGetCode(reflection.Get());
        std::vector<std::uint32_t> result(code, code + size / sizeof(std::uint32_t));
        ReflectVertexInputs(result, entryPoint);
        return result;
    }
}
