#include "UnrealProspero/Shader/ShaderBinary.h"
#include <algorithm>
#include <initializer_list>
#include <array>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <tuple>

namespace UnrealProspero::Shader
{
    namespace
    {
        constexpr std::uint32_t magic = 0x53475541;
        constexpr std::uint32_t version = 3;

        std::uint32_t narrow(std::size_t size)
        {
            if (size > std::numeric_limits<std::uint32_t>::max()) throw std::overflow_error("Shader binary exceeds format limits");
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

            std::span<const std::uint8_t> Take(std::size_t count)
            {
                if (count > remaining.size()) throw std::invalid_argument("Truncated shader binary");
                const auto result = remaining.first(count);
                remaining = remaining.subspan(count);
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

        void validate(const ShaderBinaryView& shader)
        {
            if (shader.Stage > ShaderStage::Geometry || shader.Target > AgcTarget::Ps5) throw std::invalid_argument("Invalid shader stage or target");
            if (shader.Header.empty() || shader.Code.empty() || shader.Metadata.empty() || shader.ResourceTable.empty()) throw std::invalid_argument("Incomplete shader binary");
            const bool hasVertexIdentity = std::ranges::any_of(shader.VertexIdentity, [](auto value) { return value != 0; });
            if ((shader.Stage == ShaderStage::Geometry) != hasVertexIdentity) throw std::invalid_argument("Shader has an invalid linked vertex identity");
            for (std::uint32_t index = 0; index < 8; ++index) if (((shader.ColorExportFormat >> (index * 4)) & 15u) > 9) throw std::invalid_argument("Invalid pixel shader color export format");
            if (shader.Stage != ShaderStage::Pixel && (shader.ColorExportFormat != 0 || !shader.ColorExports.empty())) throw std::invalid_argument("Color export variants require a pixel shader");
            if (shader.ColorExports.size() > 255) throw std::invalid_argument("Too many pixel shader export variants");
            for (std::size_t index = 0; index < shader.ColorExports.size(); ++index)
            {
                const auto& variant = shader.ColorExports[index];
                if (variant.Header.empty() || variant.Code.empty() || variant.Metadata.empty() || variant.Format == shader.ColorExportFormat) throw std::invalid_argument("Invalid pixel shader export variant");
                for (std::uint32_t slot = 0; slot < 8; ++slot)
                {
                    const auto format = (variant.Format >> (slot * 4)) & 15u;
                    const auto original = (shader.ColorExportFormat >> (slot * 4)) & 15u;
                    if (format > 9 || (format == 0) != (original == 0)) throw std::invalid_argument("Inconsistent pixel shader export locations");
                }
                for (std::size_t previous = 0; previous < index; ++previous) if (shader.ColorExports[previous].Format == variant.Format) throw std::invalid_argument("Duplicate pixel shader export variant");
            }
            ValidateRuntimeBindings(shader.Bindings);
        }
    }

    void ValidateLogicalBindings(std::span<const RuntimeBinding> source, std::span<const RuntimeBinding> destination)
    {
        if (source.size() != destination.size()) throw std::invalid_argument("Shader binding counts disagree");
        for (std::size_t index = 0; index < source.size(); ++index)
        {
            const auto& binding = source[index];
            const auto& expected = destination[index];
            if (std::tie(binding.Kind, binding.ParameterIndex, binding.Set, binding.Count, binding.Stride, binding.BlockSize, binding.Type) != std::tie(expected.Kind, expected.ParameterIndex, expected.Set, expected.Count, expected.Stride, expected.BlockSize, expected.Type)) throw std::invalid_argument("Shader logical bindings disagree");
        }
    }

    void ValidateGeometryVertex(const ShaderBinaryView& geometry, const ShaderBinaryView& vertex, const ShaderIdentity& identity)
    {
        validate(geometry);
        validate(vertex);
        if (geometry.Stage != ShaderStage::Geometry || vertex.Stage != ShaderStage::Vertex || geometry.Target != vertex.Target || geometry.VertexIdentity != identity) throw std::invalid_argument("Geometry shader was compiled for a different vertex shader");
    }

    void ValidateRuntimeBindings(std::span<const RuntimeBinding> bindings)
    {
        for (std::size_t index = 0; index < bindings.size(); ++index)
        {
            const auto& binding = bindings[index];
            if (binding.Set >= 4 || binding.Kind > ResourceKind::CombinedImageSampler || binding.ParameterIndex >= 65535 || binding.Count == 0 || binding.Count > 65535 - binding.ParameterIndex) throw std::invalid_argument("Invalid shader parameter range");
            const std::string_view type = binding.Type;
            std::uint32_t stride = 0;
            bool kindMatches = false;
            if (type == "sampler") { stride = 16; kindMatches = binding.Kind == ResourceKind::Sampler; }
            else if (type == "uniform-buffer") { stride = 16; kindMatches = binding.Kind == ResourceKind::UniformBuffer; }
            else if (type == "storage-buffer") { stride = 16; kindMatches = binding.Kind == ResourceKind::ShaderResource || binding.Kind == ResourceKind::UnorderedAccess; }
            else if (type == "uniform-texel-buffer") { stride = 16; kindMatches = binding.Kind == ResourceKind::ShaderResource; }
            else if (type == "storage-texel-buffer") { stride = 16; kindMatches = binding.Kind == ResourceKind::UnorderedAccess; }
            else if (type == "sampled-image" || type == "input-attachment") { stride = 32; kindMatches = binding.Kind == ResourceKind::ShaderResource; }
            else if (type == "storage-image") { stride = 32; kindMatches = binding.Kind == ResourceKind::UnorderedAccess; }
            else if (type == "combined-image-sampler") { stride = 48; kindMatches = binding.Kind == ResourceKind::CombinedImageSampler; }
            else throw std::invalid_argument("Invalid shader descriptor type");
            if (!kindMatches || binding.Stride != stride || binding.Offset % 16 != 0) throw std::invalid_argument("Inconsistent shader descriptor layout");
            if (static_cast<std::uint64_t>(binding.Offset) + static_cast<std::uint64_t>(binding.Count) * binding.Stride > std::numeric_limits<std::uint32_t>::max()) throw std::invalid_argument("Shader descriptor range overflow");
            if ((binding.Kind == ResourceKind::UniformBuffer) != (binding.BlockSize != 0)) throw std::invalid_argument("Invalid shader constant buffer size");
            for (std::size_t previous = 0; previous < index; ++previous)
            {
                const auto& other = bindings[previous];
                if (binding.Set == other.Set)
                {
                    if (binding.Binding == other.Binding) throw std::invalid_argument("Duplicate shader descriptor binding");
                    const auto end = static_cast<std::uint64_t>(binding.Offset) + static_cast<std::uint64_t>(binding.Count) * binding.Stride;
                    const auto otherEnd = static_cast<std::uint64_t>(other.Offset) + static_cast<std::uint64_t>(other.Count) * other.Stride;
                    if (binding.Offset < otherEnd && other.Offset < end) throw std::invalid_argument("Overlapping shader descriptor ranges");
                }
                if (binding.Kind == other.Kind && binding.ParameterIndex < other.ParameterIndex + other.Count && other.ParameterIndex < binding.ParameterIndex + binding.Count) throw std::invalid_argument("Overlapping shader parameter ranges");
            }
        }
    }

    std::vector<std::uint8_t> EncodeShaderBinary(const ShaderBinaryView& shader)
    {
        validate(shader);
        std::vector<std::uint8_t> result;
        for (auto value : {magic, version, static_cast<std::uint32_t>(shader.Stage), static_cast<std::uint32_t>(shader.Target), narrow(shader.Bindings.size())}) append(result, value);
        for (auto section : {shader.Header, shader.Code, shader.Metadata, shader.ResourceTable}) append(result, narrow(section.size()));
        result.insert(result.end(), shader.VertexIdentity.begin(), shader.VertexIdentity.end());
        append(result, shader.ColorExportFormat);
        append(result, narrow(shader.ColorExports.size()));
        for (const auto& binding : shader.Bindings)
        {
            for (auto value : {static_cast<std::uint32_t>(binding.Kind), binding.ParameterIndex, binding.Set, binding.Binding, binding.Count, binding.Offset, binding.Stride, binding.BlockSize, narrow(binding.Type.size())}) append(result, value);
            result.insert(result.end(), binding.Type.begin(), binding.Type.end());
        }
        for (const auto& variant : shader.ColorExports)
        {
            append(result, variant.Format);
            for (auto section : {variant.Header, variant.Code, variant.Metadata}) append(result, narrow(section.size()));
        }
        for (auto section : {shader.Header, shader.Code, shader.Metadata}) result.insert(result.end(), section.begin(), section.end());
        for (const auto& variant : shader.ColorExports) for (auto section : {variant.Header, variant.Code, variant.Metadata}) result.insert(result.end(), section.begin(), section.end());
        result.insert(result.end(), shader.ResourceTable.begin(), shader.ResourceTable.end());
        narrow(result.size());
        return result;
    }

    ShaderBinaryView DecodeShaderBinary(std::span<const std::uint8_t> bytes)
    {
        narrow(bytes.size());
        Reader reader(bytes);
        if (reader.Read() != magic || reader.Read() != version) throw std::invalid_argument("Unsupported shader binary format");
        ShaderBinaryView result{};
        result.Stage = static_cast<ShaderStage>(reader.Read());
        result.Target = static_cast<AgcTarget>(reader.Read());
        const auto count = reader.Read();
        const auto headerSize = reader.Read();
        const auto codeSize = reader.Read();
        const auto metadataSize = reader.Read();
        const auto resourceSize = reader.Read();
        const auto vertexIdentity = reader.Take(result.VertexIdentity.size());
        std::copy(vertexIdentity.begin(), vertexIdentity.end(), result.VertexIdentity.begin());
        result.ColorExportFormat = reader.Read();
        const auto variantCount = reader.Read();
        if (variantCount > 255 || variantCount > reader.Size() / 16) throw std::invalid_argument("Invalid color export variant count");
        const auto payloadSize = static_cast<std::uint64_t>(headerSize) + codeSize + metadataSize + resourceSize + static_cast<std::uint64_t>(variantCount) * 16;
        if (payloadSize > reader.Size() || count > (reader.Size() - payloadSize) / 37) throw std::invalid_argument("Invalid shader binary section sizes");
        result.Bindings.reserve(count);
        for (std::uint32_t index = 0; index < count; ++index)
        {
            RuntimeBinding binding{};
            binding.Kind = static_cast<ResourceKind>(reader.Read());
            binding.ParameterIndex = reader.Read();
            binding.Set = reader.Read();
            binding.Binding = reader.Read();
            binding.Count = reader.Read();
            binding.Offset = reader.Read();
            binding.Stride = reader.Read();
            binding.BlockSize = reader.Read();
            const auto type = reader.Take(reader.Read());
            binding.Type.assign(type.begin(), type.end());
            result.Bindings.push_back(std::move(binding));
        }
        std::vector<std::array<std::uint32_t, 4>> variants;
        variants.reserve(variantCount);
        std::uint64_t variantBytes = 0;
        for (std::uint32_t index = 0; index < variantCount; ++index)
        {
            std::array<std::uint32_t, 4> entry;
            for (auto& value : entry) value = reader.Read();
            variantBytes += static_cast<std::uint64_t>(entry[1]) + entry[2] + entry[3];
            variants.push_back(entry);
        }
        if (variantBytes + headerSize + codeSize + metadataSize + resourceSize > reader.Size()) throw std::invalid_argument("Truncated color export variant payload");
        result.Header = reader.Take(headerSize);
        result.Code = reader.Take(codeSize);
        result.Metadata = reader.Take(metadataSize);
        result.ColorExports.reserve(variantCount);
        for (const auto& entry : variants)
        {
            const auto header = reader.Take(entry[1]);
            const auto code = reader.Take(entry[2]);
            const auto metadata = reader.Take(entry[3]);
            result.ColorExports.push_back({entry[0], header, code, metadata});
        }
        result.ResourceTable = reader.Take(resourceSize);
        if (reader.Size() != 0) throw std::invalid_argument("Trailing shader binary data");
        validate(result);
        return result;
    }

    UnrealShaderCodeView DecodeUnrealShaderEnvelope(std::span<const std::uint8_t> bytes)
    {
        if (bytes.size() < 4 || bytes.size() > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) throw std::invalid_argument("Invalid Unreal shader code size");
        Reader trailer(bytes.last(4));
        const auto optionalSize = trailer.Read();
        if (optionalSize < 4 || optionalSize >= bytes.size()) throw std::invalid_argument("Invalid Unreal shader optional data size");
        const auto codeSize = bytes.size() - optionalSize;
        Reader optional(bytes.subspan(codeSize, optionalSize - 4));
        UnrealShaderCodeView result{bytes.first(codeSize), {}};
        std::array<bool, 256> keys{};
        while (optional.Size() != 0)
        {
            const auto key = optional.Take(1).front();
            if (keys[key]) throw std::invalid_argument("Duplicate Unreal shader optional data key");
            keys[key] = true;
            result.OptionalData.push_back({key, optional.Take(optional.Read())});
        }
        return result;
    }

    std::vector<std::uint8_t> EncodeUnrealShaderEnvelope(const UnrealShaderCodeView& shader)
    {
        if (shader.Code.empty()) throw std::invalid_argument("Unreal shader code is empty");
        if (shader.Code.size() > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) throw std::overflow_error("Unreal shader code exceeds array limits");
        std::array<bool, 256> keys{};
        std::uint64_t optionalSize = 4;
        for (const auto& data : shader.OptionalData)
        {
            if (keys[data.Key]) throw std::invalid_argument("Duplicate Unreal shader optional data key");
            keys[data.Key] = true;
            optionalSize += std::uint64_t{5} + narrow(data.Bytes.size());
        }
        const auto size = optionalSize + shader.Code.size();
        if (size > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())) throw std::overflow_error("Unreal shader code exceeds array limits");
        std::vector<std::uint8_t> result;
        result.reserve(static_cast<std::size_t>(size));
        result.insert(result.end(), shader.Code.begin(), shader.Code.end());
        for (const auto& data : shader.OptionalData)
        {
            result.push_back(data.Key);
            append(result, narrow(data.Bytes.size()));
            result.insert(result.end(), data.Bytes.begin(), data.Bytes.end());
        }
        append(result, static_cast<std::uint32_t>(optionalSize));
        return result;
    }

    ShaderBinaryView DecodeUnrealShaderCode(std::span<const std::uint8_t> bytes)
    {
        return DecodeShaderBinary(DecodeUnrealShaderEnvelope(bytes).Code);
    }
}
