#include "AnyUnreal/RHI/Shader/AgcHeader.h"
#include "SceShaders.hpp"
#include <cstddef>
#include <stdexcept>
#include <type_traits>

namespace AnyUnreal::RHI
{
    namespace
    {
        class HeaderReader final
        {
        public:
            explicit HeaderReader(std::span<const std::uint8_t> bytes) : bytes(bytes) {}

            template<class TValue>
            TValue Read(std::size_t offset) const
            {
                static_assert(std::is_unsigned_v<TValue>);
                if (offset > bytes.size() || sizeof(TValue) > bytes.size() - offset) throw std::invalid_argument("Truncated AGC header");
                std::uint64_t value = 0;
                for (std::size_t index = 0; index < sizeof(TValue); ++index) value |= static_cast<std::uint64_t>(bytes[offset + index]) << (index * 8);
                return static_cast<TValue>(value);
            }

            std::size_t Pointer(std::size_t field, std::size_t count, std::size_t stride, std::size_t alignment) const
            {
                const auto relative = Read<std::uint64_t>(field);
                if (relative == 0)
                {
                    if (count != 0) throw std::invalid_argument("Missing AGC header table");
                    return 0;
                }
                const auto offset = static_cast<std::uint64_t>(field) + relative;
                if (offset < sizeof(::Shader) || offset > bytes.size() || offset % alignment != 0 || count > (bytes.size() - offset) / stride) throw std::invalid_argument("AGC relative pointer exceeds its header");
                return static_cast<std::size_t>(offset);
            }

        private:
            std::span<const std::uint8_t> bytes;
        };
    }

    void ValidateAgcHeader(std::span<const std::uint8_t> header, std::span<const std::uint8_t> code, Shader::ShaderStage stage)
    {
        if (header.size() < sizeof(::Shader) || header.size() % alignof(::Shader) != 0 || code.empty() || code.size() % 4 != 0) throw std::invalid_argument("Invalid AGC header or code size");
        HeaderReader reader(header);
        if (reader.Read<std::uint32_t>(offsetof(::Shader, file_header)) != 0x34333231 || reader.Read<std::uint32_t>(offsetof(::Shader, version)) != 0x18) throw std::invalid_argument("Unsupported AGC header version");
        if (reader.Read<std::uint32_t>(offsetof(::Shader, header_size)) != header.size() || reader.Read<std::uint32_t>(offsetof(::Shader, shader_size)) != code.size()) throw std::invalid_argument("AGC header and code sizes disagree");
        if (reader.Read<std::uint64_t>(offsetof(::Shader, code)) != 0) throw std::invalid_argument("AGC header has already been relocated");
        if (reader.Read<std::uint32_t>(offsetof(::Shader, target)) != 5) throw std::invalid_argument("Unsupported AGC hardware target");
        const auto type = reader.Read<std::uint8_t>(offsetof(::Shader, type));
        const bool validStage = ((stage == Shader::ShaderStage::Vertex || stage == Shader::ShaderStage::Geometry) && type == 2) || (stage == Shader::ShaderStage::Pixel && type == 1) || (stage == Shader::ShaderStage::Compute && type == 0);
        if (!validStage) throw std::invalid_argument("AGC shader stage does not match its binary envelope");
        const auto shaderRegisters = reader.Read<std::uint8_t>(offsetof(::Shader, num_sh_registers));
        if (shaderRegisters == 0) throw std::invalid_argument("AGC shader has no program registers");
        reader.Pointer(offsetof(::Shader, sh_registers), shaderRegisters, sizeof(ShaderRegister), alignof(ShaderRegister));
        reader.Pointer(offsetof(::Shader, cx_registers), reader.Read<std::uint8_t>(offsetof(::Shader, num_cx_registers)), sizeof(ShaderRegister), alignof(ShaderRegister));
        reader.Pointer(offsetof(::Shader, input_semantics), reader.Read<std::uint32_t>(offsetof(::Shader, num_input_semantics)), sizeof(ShaderSemantic), alignof(ShaderSemantic));
        reader.Pointer(offsetof(::Shader, output_semantics), reader.Read<std::uint16_t>(offsetof(::Shader, num_output_semantics)), sizeof(ShaderSemantic), alignof(ShaderSemantic));
        const auto specialSize = reader.Read<std::uint16_t>(offsetof(::Shader, special_sizes_bytes));
        if (specialSize != 0 && specialSize != sizeof(ShaderSpecialRegs)) throw std::invalid_argument("Unsupported AGC special register layout");
        reader.Pointer(offsetof(::Shader, specials), specialSize, 1, alignof(ShaderSpecialRegs));
        if (stage != Shader::ShaderStage::Pixel && specialSize == 0) throw std::invalid_argument("AGC shader is missing stage registers");
        const auto resources = reader.Pointer(offsetof(::Shader, user_data), reader.Read<std::uint64_t>(offsetof(::Shader, user_data)) == 0 ? 0 : 1, sizeof(ShaderUserData), alignof(ShaderUserData));
        if (resources != 0)
        {
            reader.Pointer(resources + offsetof(ShaderUserData, direct_resource_offset), reader.Read<std::uint16_t>(resources + offsetof(ShaderUserData, direct_resource_count)), sizeof(std::uint16_t), alignof(std::uint16_t));
            for (std::size_t index = 0; index < 4; ++index)
            {
                const auto count = reader.Read<std::uint16_t>(resources + offsetof(ShaderUserData, sharp_resource_count) + index * sizeof(std::uint16_t));
                reader.Pointer(resources + offsetof(ShaderUserData, sharp_resource_offset) + index * sizeof(std::uint64_t), count, sizeof(ShaderSharp), alignof(ShaderSharp));
            }
        }
    }
}
