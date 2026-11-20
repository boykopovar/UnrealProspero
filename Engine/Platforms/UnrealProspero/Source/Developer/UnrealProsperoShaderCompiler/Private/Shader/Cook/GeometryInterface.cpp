#include "UnrealProspero/Shader/Cook/GeometryInterface.h"
#include "UnrealProspero/Shader/SpirvReflection.h"
#include <limits>
#include <charconv>
#include <map>
#include <stdexcept>
#include <utility>

namespace UnrealProspero::Shader::Cook
{
    namespace
    {
        using Remap = std::pair<const SpvReflectInterfaceVariable*, std::uint32_t>;

        std::string semantic(const SpvReflectInterfaceVariable& variable)
        {
            if (!variable.semantic || !*variable.semantic) throw std::invalid_argument("Shader interface has no HLSL semantic");
            std::string result(variable.semantic);
            for (auto& character : result)
            {
                if (character >= 'a' && character <= 'z') character = static_cast<char>(character - 'a' + 'A');
            }
            const auto suffix = result.find_last_not_of("0123456789");
            if (suffix == std::string::npos) throw std::invalid_argument("Shader semantic has no identifier");
            const auto begin = suffix + 1;
            std::uint32_t index = 0;
            if (begin != result.size())
            {
                const auto parsed = std::from_chars(result.data() + begin, result.data() + result.size(), index);
                if (parsed.ec != std::errc{} || parsed.ptr != result.data() + result.size()) throw std::invalid_argument("Invalid shader semantic index");
            }
            result.resize(begin);
            return result + std::to_string(index);
        }

        bool matches(const SpvReflectInterfaceVariable& output, const SpvReflectInterfaceVariable& input)
        {
            if (!output.type_description || !input.type_description || output.member_count != 0 || input.member_count != 0) throw std::invalid_argument("Geometry linking requires flattened interface types");
            constexpr auto scalarMask = SPV_REFLECT_TYPE_FLAG_FLOAT | SPV_REFLECT_TYPE_FLAG_INT | SPV_REFLECT_TYPE_FLAG_BOOL;
            if ((output.type_description->type_flags & scalarMask) != (input.type_description->type_flags & scalarMask)) return false;
            if (output.numeric.scalar.width != input.numeric.scalar.width || output.numeric.scalar.signedness != input.numeric.scalar.signedness || output.numeric.vector.component_count < input.numeric.vector.component_count || output.numeric.matrix.column_count != input.numeric.matrix.column_count || output.numeric.matrix.row_count != input.numeric.matrix.row_count || output.component != input.component) return false;
            if (input.array.dims_count != output.array.dims_count + 1) return false;
            for (std::uint32_t index = 0; index < input.array.dims_count; ++index)
            {
                if (input.array.dims[index] == 0 || input.array.spec_constant_op_ids[index] != std::numeric_limits<std::uint32_t>::max()) throw std::invalid_argument("Geometry interface array requires a constant dimension");
                if (index != 0)
                {
                    if (output.array.spec_constant_op_ids[index - 1] != std::numeric_limits<std::uint32_t>::max()) throw std::invalid_argument("Vertex interface array requires a constant dimension");
                    if (input.array.dims[index] != output.array.dims[index - 1]) return false;
                }
            }
            return true;
        }

        bool prepare(const SpirvReflection& vertex, const std::string& vertexEntry, const SpirvReflection& geometry, const std::string& geometryEntry, std::vector<Remap>& remaps)
        {
            const auto* producer = spvReflectGetEntryPoint(vertex.Get(), vertexEntry.c_str());
            const auto* consumer = spvReflectGetEntryPoint(geometry.Get(), geometryEntry.c_str());
            if (!producer || producer->shader_stage != SPV_REFLECT_SHADER_STAGE_VERTEX_BIT || !consumer || consumer->shader_stage != SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT) throw std::invalid_argument("Geometry linking requires vertex and geometry entry points");
            std::map<std::string, const SpvReflectInterfaceVariable*> semantics;
            std::map<SpvBuiltIn, const SpvReflectInterfaceVariable*> builtins;
            for (std::uint32_t index = 0; index < producer->output_variable_count; ++index)
            {
                const auto* output = producer->output_variables[index];
                if (output->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN)
                {
                    if (!builtins.emplace(output->built_in, output).second) throw std::invalid_argument("Duplicate vertex output builtin");
                }
                else if (!semantics.emplace(semantic(*output), output).second) throw std::invalid_argument("Duplicate vertex output semantic");
            }
            for (std::uint32_t index = 0; index < consumer->input_variable_count; ++index)
            {
                const auto* input = consumer->input_variables[index];
                const SpvReflectInterfaceVariable* output = nullptr;
                if (input->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN)
                {
                    if (input->built_in == SpvBuiltInPrimitiveId || input->built_in == SpvBuiltInInvocationId) continue;
                    const auto found = builtins.find(input->built_in);
                    if (found != builtins.end()) output = found->second;
                }
                else
                {
                    const auto found = semantics.find(semantic(*input));
                    if (found != semantics.end()) output = found->second;
                }
                if (!output || !matches(*output, *input)) return false;
                if (!(input->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN))
                {
                    if (output->location >= 32) throw std::invalid_argument("Geometry input location exceeds the AGC interface range");
                    remaps.emplace_back(input, output->location);
                }
            }
            return true;
        }
    }

    bool CanLinkGeometryStages(std::span<const std::uint32_t> vertexWords, const std::string& vertexEntry, std::span<const std::uint32_t> geometryWords, const std::string& geometryEntry)
    {
        const SpirvReflection vertex(vertexWords);
        const SpirvReflection geometry(geometryWords);
        std::vector<Remap> remaps;
        return prepare(vertex, vertexEntry, geometry, geometryEntry, remaps);
    }

    std::vector<std::uint32_t> LinkGeometryInputs(std::span<const std::uint32_t> vertexWords, const std::string& vertexEntry, std::span<const std::uint32_t> geometryWords, const std::string& geometryEntry)
    {
        const SpirvReflection vertex(vertexWords);
        SpirvReflection geometry(geometryWords);
        std::vector<Remap> remaps;
        if (!prepare(vertex, vertexEntry, geometry, geometryEntry, remaps)) throw std::invalid_argument("Vertex outputs do not satisfy the geometry input interface");
        for (const auto& [input, location] : remaps) geometry.RemapInputLocation(input, location);
        const auto size = spvReflectGetCodeSize(geometry.Get());
        if (size % sizeof(std::uint32_t) != 0) throw std::runtime_error("Invalid linked geometry SPIR-V size");
        const auto* code = spvReflectGetCode(geometry.Get());
        return {code, code + size / sizeof(std::uint32_t)};
    }
}
