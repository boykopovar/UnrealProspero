#include "AnyUnreal/Shader/ShaderBinary.h"
#include <initializer_list>
#include <array>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace AnyUnreal::Shader
{
    namespace
    {
        constexpr std::uint32_t magic = 0x53475541;
        constexpr std::uint32_t version = 1;

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
            ValidateRuntimeBindings(shader.Bindings);
        }
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
        for (const auto& binding : shader.Bindings)
        {
            for (auto value : {static_cast<std::uint32_t>(binding.Kind), binding.ParameterIndex, binding.Set, binding.Binding, binding.Count, binding.Offset, binding.Stride, binding.BlockSize, narrow(binding.Type.size())}) append(result, value);
            result.insert(result.end(), binding.Type.begin(), binding.Type.end());
        }
        for (auto section : {shader.Header, shader.Code, shader.Metadata, shader.ResourceTable}) result.insert(result.end(), section.begin(), section.end());
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
        const auto payloadSize = static_cast<std::uint64_t>(headerSize) + codeSize + metadataSize + resourceSize;
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
        result.Header = reader.Take(headerSize);
        result.Code = reader.Take(codeSize);
        result.Metadata = reader.Take(metadataSize);
        result.ResourceTable = reader.Take(resourceSize);
        if (reader.Size() != 0) throw std::invalid_argument("Trailing shader binary data");
        validate(result);
        return result;
    }

    ShaderBinaryView DecodeUnrealShaderCode(std::span<const std::uint8_t> bytes)
    {
        if (bytes.size() < 4 || bytes.size() > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) throw std::invalid_argument("Invalid Unreal shader code size");
        Reader trailer(bytes.last(4));
        const auto optionalSize = trailer.Read();
        if (optionalSize < 4 || optionalSize >= bytes.size()) throw std::invalid_argument("Invalid Unreal shader optional data size");
        const auto codeSize = bytes.size() - optionalSize;
        Reader optional(bytes.subspan(codeSize, optionalSize - 4));
        std::array<bool, 256> keys{};
        while (optional.Size() != 0)
        {
            const auto key = optional.Take(1).front();
            if (keys[key]) throw std::invalid_argument("Duplicate Unreal shader optional data key");
            keys[key] = true;
            optional.Take(optional.Read());
        }
        return DecodeShaderBinary(bytes.first(codeSize));
    }
}
