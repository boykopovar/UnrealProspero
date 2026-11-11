#include "AnyUnreal/Shader/VertexInputs.h"
#include "AnyUnreal/Shader/SpirvReflection.h"
#include <algorithm>
#include <stdexcept>

namespace AnyUnreal::Shader
{
    namespace
    {
        void append(const SpvReflectInterfaceVariable& input, std::vector<VertexInput>& result)
        {
            if (input.decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) return;
            if (input.member_count != 0)
            {
                for (std::uint32_t index = 0; index < input.member_count; ++index) append(input.members[index], result);
                return;
            }
            if (!input.type_description || input.numeric.scalar.width != 32) throw std::invalid_argument("Vertex input requires a 32-bit scalar type");
            const auto flags = input.type_description->type_flags;
            VertexScalarType type;
            if (flags & SPV_REFLECT_TYPE_FLAG_FLOAT) type = VertexScalarType::Float;
            else if (flags & SPV_REFLECT_TYPE_FLAG_INT) type = input.numeric.scalar.signedness ? VertexScalarType::SignedInteger : VertexScalarType::UnsignedInteger;
            else throw std::invalid_argument("Unsupported vertex input scalar type");
            const auto components = input.numeric.matrix.column_count ? input.numeric.matrix.row_count : std::max(1u, input.numeric.vector.component_count);
            if (components == 0 || components > 4) throw std::invalid_argument("Invalid vertex input component count");
            std::uint64_t locations = std::max(1u, input.numeric.matrix.column_count);
            for (std::uint32_t index = 0; index < input.array.dims_count; ++index)
            {
                if (input.array.dims[index] == 0 || locations > 32 / input.array.dims[index]) throw std::invalid_argument("Vertex input array exceeds the attribute limit");
                locations *= input.array.dims[index];
            }
            if (input.location >= 32 || locations > 32 - input.location) throw std::out_of_range("Vertex input locations exceed the attribute limit");
            for (std::uint32_t index = 0; index < locations; ++index) result.push_back({input.location + index, components, type});
        }
    }

    std::vector<VertexInput> ReflectVertexInputs(std::span<const std::uint32_t> words, const std::string& entryPoint)
    {
        if (entryPoint.empty() || entryPoint.find('\0') != std::string::npos) throw std::invalid_argument("Invalid shader entry point");
        SpirvReflection reflection(words);
        const auto* entry = spvReflectGetEntryPoint(reflection.Get(), entryPoint.c_str());
        if (!entry || entry->shader_stage != SPV_REFLECT_SHADER_STAGE_VERTEX_BIT) throw std::invalid_argument("Vertex reflection requires a vertex entry point");
        std::uint32_t count = 0;
        SpirvReflection::Check(spvReflectEnumerateEntryPointInputVariables(reflection.Get(), entryPoint.c_str(), &count, nullptr));
        std::vector<SpvReflectInterfaceVariable*> inputs(count);
        if (count) SpirvReflection::Check(spvReflectEnumerateEntryPointInputVariables(reflection.Get(), entryPoint.c_str(), &count, inputs.data()));
        std::vector<VertexInput> result;
        for (const auto* input : inputs) append(*input, result);
        std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) { return left.Location < right.Location; });
        for (std::size_t index = 1; index < result.size(); ++index)
        {
            if (result[index - 1].Location == result[index].Location) throw std::invalid_argument("Vertex inputs overlap locations");
        }
        return result;
    }
}
