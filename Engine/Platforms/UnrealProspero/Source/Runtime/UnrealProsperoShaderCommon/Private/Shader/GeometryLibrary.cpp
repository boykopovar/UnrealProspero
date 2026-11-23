#include "UnrealProspero/Shader/GeometryLibrary.h"
#include <algorithm>
#include <limits>
#include <initializer_list>
#include <set>
#include <stdexcept>
#include <utility>

namespace UnrealProspero::Shader
{
    namespace
    {
        constexpr std::uint32_t Magic = 0x56474741;
        constexpr std::uint32_t Version = 1;

        std::uint32_t narrowLibrarySize(std::size_t size)
        {
            if (size > std::numeric_limits<std::uint32_t>::max()) throw std::overflow_error("Geometry library exceeds format limits");
            return static_cast<std::uint32_t>(size);
        }

        void appendLibraryWord(std::vector<std::uint8_t>& bytes, std::uint32_t value)
        {
            for (unsigned shift = 0; shift < 32; shift += 8) bytes.push_back(static_cast<std::uint8_t>(value >> shift));
        }

        class GeometryLibraryReader final
        {
        public:
            explicit GeometryLibraryReader(std::span<const std::uint8_t> bytes) : remaining(bytes) {}

            std::span<const std::uint8_t> Take(std::size_t size)
            {
                if (size > remaining.size()) throw std::invalid_argument("Truncated geometry library");
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

        void validate(const ShaderBinaryView& shader, const ShaderBinaryView& first)
        {
            if (shader.Stage != ShaderStage::Geometry || shader.Target != first.Target) throw std::invalid_argument("Geometry library contains an incompatible stage or target");
            if (!std::ranges::equal(shader.ResourceTable, first.ResourceTable) || shader.Bindings.size() != first.Bindings.size()) throw std::invalid_argument("Geometry library variants disagree on their resource interface");
            ValidateLogicalBindings(shader.Bindings, first.Bindings);
        }
    }

    std::vector<std::uint8_t> EncodeGeometryLibrary(std::span<const std::vector<std::uint8_t>> variants)
    {
        if (variants.empty()) throw std::invalid_argument("Geometry library has no variants");
        std::uint64_t size = 12;
        const auto count = narrowLibrarySize(variants.size());
        const auto first = DecodeShaderBinary(variants.front());
        std::set<ShaderIdentity> identities;
        for (const auto& bytes : variants)
        {
            const auto shader = DecodeShaderBinary(bytes);
            validate(shader, first);
            if (!identities.insert(shader.VertexIdentity).second) throw std::invalid_argument("Duplicate geometry library vertex identity");
            size += std::uint64_t{4} + narrowLibrarySize(bytes.size());
            if (size > std::numeric_limits<std::uint32_t>::max()) throw std::overflow_error("Geometry library exceeds format limits");
        }
        std::vector<std::uint8_t> result;
        result.reserve(static_cast<std::size_t>(size));
        for (const auto value : {Magic, Version, count}) appendLibraryWord(result, value);
        for (const auto& bytes : variants)
        {
            appendLibraryWord(result, narrowLibrarySize(bytes.size()));
            result.insert(result.end(), bytes.begin(), bytes.end());
        }
        return result;
    }

    std::vector<GeometryVariantView> DecodeGeometryLibrary(std::span<const std::uint8_t> bytes)
    {
        if (bytes.size() > std::numeric_limits<std::uint32_t>::max()) throw std::invalid_argument("Geometry library exceeds format limits");
        GeometryLibraryReader reader(bytes);
        if (reader.Read() != Magic || reader.Read() != Version) throw std::invalid_argument("Unsupported geometry library format");
        const auto count = reader.Read();
        if (count == 0 || count > reader.Size() / 4) throw std::invalid_argument("Invalid geometry library variant count");
        std::vector<GeometryVariantView> result;
        std::set<ShaderIdentity> identities;
        for (std::uint32_t index = 0; index < count; ++index)
        {
            const auto code = reader.Take(reader.Read());
            auto shader = DecodeShaderBinary(code);
            validate(shader, result.empty() ? shader : result.front().Binary);
            if (!identities.insert(shader.VertexIdentity).second) throw std::invalid_argument("Duplicate geometry library vertex identity");
            result.push_back({code, std::move(shader)});
        }
        if (reader.Size() != 0) throw std::invalid_argument("Trailing geometry library data");
        return result;
    }
}
