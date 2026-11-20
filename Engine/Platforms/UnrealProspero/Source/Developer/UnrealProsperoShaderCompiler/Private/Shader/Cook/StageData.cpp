#include "UnrealProspero/Shader/Cook/StageData.h"
#include "UnrealProspero/Shader/ResourceTableData.h"
#include "UnrealProspero/Shader/ShaderOutput.h"
#include "UnrealProspero/Shader/SpirvReflection.h"
#include <limits>
#include <stdexcept>

namespace UnrealProspero::Shader::Cook
{
    namespace
    {
        constexpr std::uint32_t Magic = 0x43534741;
        constexpr std::uint32_t Version = 1;

        std::uint32_t narrow(std::size_t size)
        {
            if (size > std::numeric_limits<std::uint32_t>::max()) throw std::overflow_error("Shader cook data exceeds format limits");
            return static_cast<std::uint32_t>(size);
        }

        void append(std::vector<std::uint8_t>& bytes, std::uint32_t value)
        {
            for (unsigned shift = 0; shift < 32; shift += 8) bytes.push_back(static_cast<std::uint8_t>(value >> shift));
        }

        class Reader final
        {
        public:
            explicit Reader(std::span<const std::uint8_t> bytes) : remaining(bytes) {}

            std::span<const std::uint8_t> Take(std::size_t size)
            {
                if (size > remaining.size()) throw std::invalid_argument("Truncated shader cook data");
                const auto result = remaining.first(size);
                remaining = remaining.subspan(size);
                return result;
            }

            std::uint32_t Read()
            {
                const auto bytes = Take(4);
                std::uint32_t result = 0;
                for (unsigned index = 0; index < 4; ++index) result |= static_cast<std::uint32_t>(bytes[index]) << (index * 8);
                return result;
            }

            std::size_t Size() const { return remaining.size(); }

        private:
            std::span<const std::uint8_t> remaining;
        };

        std::string entryPoint(const FString& value)
        {
            const FTCHARToUTF8 encoded(*value, value.Len());
            std::string result(encoded.Get(), static_cast<std::size_t>(encoded.Length()));
            if (result.empty() || result.find('\0') != std::string::npos || FString(UTF8_TO_TCHAR(result.c_str())) != value) throw std::invalid_argument("Invalid shader cook entry point");
            return result;
        }

        void validate(const StageData& data)
        {
            if (data.Stage != SF_Vertex && data.Stage != SF_Geometry) throw std::invalid_argument("Shader cook data requires a vertex or geometry stage");
            if (data.Target > AgcTarget::Ps5) throw std::invalid_argument("Invalid shader cook target");
            const auto name = entryPoint(data.EntryPoint);
            const SpirvReflection reflection(data.Spirv.Words);
            const auto* entry = spvReflectGetEntryPoint(reflection.Get(), name.c_str());
            const auto expected = data.Stage == SF_Vertex ? SPV_REFLECT_SHADER_STAGE_VERTEX_BIT : SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT;
            if (!entry || entry->shader_stage != expected) throw std::invalid_argument("Shader cook execution model does not match its stage");
            if (ReflectDescriptorLayout(data.Spirv.Words, name) != data.Spirv.Descriptors) throw std::invalid_argument("Shader cook descriptors disagree with SPIR-V");
            if (data.Stage == SF_Vertex && ReflectVertexInputs(data.Spirv.Words, name) != data.Spirv.VertexInputs) throw std::invalid_argument("Shader cook vertex inputs disagree with SPIR-V");
            if (data.Stage == SF_Geometry && !data.Spirv.VertexInputs.empty()) throw std::invalid_argument("Geometry cook data contains vertex inputs");
            if (data.Spirv.Parameters.size() != data.Spirv.Descriptors.size()) throw std::invalid_argument("Shader cook parameters do not cover its descriptors");
            std::vector<bool> seen(data.Spirv.Descriptors.size());
            for (const auto& parameter : data.Spirv.Parameters)
            {
                if (parameter.DescriptorIndex >= seen.size() || seen[parameter.DescriptorIndex]) throw std::invalid_argument("Invalid or duplicate shader cook parameter");
                seen[parameter.DescriptorIndex] = true;
            }
            BuildRuntimeBindings(data.Spirv);
        }
    }

    std::vector<std::uint8_t> EncodeStageData(const StageData& data)
    {
        validate(data);
        const auto name = entryPoint(data.EntryPoint);
        const auto table = EncodeResourceTable(data.Spirv.ResourceTable);
        const auto nameSize = narrow(name.size());
        const auto wordCount = narrow(data.Spirv.Words.size());
        const auto parameterCount = narrow(data.Spirv.Parameters.size());
        const auto tableSize = narrow(table.size());
        const auto size = std::uint64_t{32} + nameSize + std::uint64_t{wordCount} * 4 + std::uint64_t{parameterCount} * 8 + tableSize;
        if (size > static_cast<std::uint64_t>(std::numeric_limits<int32>::max())) throw std::overflow_error("Shader cook data exceeds Unreal array limits");
        std::vector<std::uint8_t> result;
        result.reserve(static_cast<std::size_t>(size));
        for (const auto value : {Magic, Version, static_cast<std::uint32_t>(data.Stage), static_cast<std::uint32_t>(data.Target), nameSize, wordCount, parameterCount, tableSize}) append(result, value);
        result.insert(result.end(), name.begin(), name.end());
        for (const auto word : data.Spirv.Words) append(result, word);
        for (const auto& parameter : data.Spirv.Parameters)
        {
            append(result, narrow(parameter.DescriptorIndex));
            append(result, parameter.ParameterIndex);
        }
        result.insert(result.end(), table.begin(), table.end());
        return result;
    }

    StageData DecodeStageData(std::span<const std::uint8_t> bytes)
    {
        if (bytes.size() > static_cast<std::size_t>(std::numeric_limits<int32>::max())) throw std::invalid_argument("Shader cook data exceeds Unreal array limits");
        Reader reader(bytes);
        if (reader.Read() != Magic || reader.Read() != Version) throw std::invalid_argument("Unsupported shader cook data format");
        StageData result{};
        const auto stage = reader.Read();
        if (stage != SF_Vertex && stage != SF_Geometry) throw std::invalid_argument("Invalid shader cook stage");
        result.Stage = static_cast<EShaderFrequency>(stage);
        result.Target = static_cast<AgcTarget>(reader.Read());
        const auto nameSize = reader.Read();
        const auto wordCount = reader.Read();
        const auto parameterCount = reader.Read();
        const auto tableSize = reader.Read();
        const auto nameBytes = reader.Take(nameSize);
        const std::string name(reinterpret_cast<const char*>(nameBytes.data()), nameBytes.size());
        if (name.empty() || name.find('\0') != std::string::npos) throw std::invalid_argument("Invalid shader cook entry point");
        result.EntryPoint = UTF8_TO_TCHAR(name.c_str());
        if (entryPoint(result.EntryPoint) != name) throw std::invalid_argument("Invalid UTF-8 shader cook entry point");
        if (wordCount > reader.Size() / 4) throw std::invalid_argument("Truncated shader cook SPIR-V");
        result.Spirv.Words.reserve(wordCount);
        for (std::uint32_t index = 0; index < wordCount; ++index) result.Spirv.Words.push_back(reader.Read());
        if (parameterCount > reader.Size() / 8) throw std::invalid_argument("Truncated shader cook parameters");
        result.Spirv.Parameters.reserve(parameterCount);
        for (std::uint32_t index = 0; index < parameterCount; ++index)
        {
            const auto descriptor = reader.Read();
            const auto parameter = reader.Read();
            if (parameter >= std::numeric_limits<std::uint16_t>::max()) throw std::invalid_argument("Shader cook parameter exceeds Unreal limits");
            result.Spirv.Parameters.push_back({descriptor, static_cast<std::uint16_t>(parameter)});
        }
        DecodeResourceTable(reader.Take(tableSize), result.Spirv.ResourceTable);
        if (reader.Size() != 0) throw std::invalid_argument("Trailing shader cook data");
        result.Spirv.Descriptors = ReflectDescriptorLayout(result.Spirv.Words, name);
        if (result.Stage == SF_Vertex) result.Spirv.VertexInputs = ReflectVertexInputs(result.Spirv.Words, name);
        validate(result);
        return result;
    }
}
