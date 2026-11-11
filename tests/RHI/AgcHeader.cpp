#include "AnyUnreal/RHI/Shader/AgcHeader.h"
#include "AnyUnreal/Shader/UserData.h"
#include "SceShaders.hpp"
#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <string_view>

#if ANYUNREAL_TEST_CREATE_SHADER
#include "AnyUnreal/RHI/Shader/AgcShader.h"
#include "AnyUnreal/RHI/Shader/ShaderCommands.h"
#include "AnyUnreal/RHI/Shader/GraphicsShaderLink.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libSceAgc/libSceAgc.hpp"
#include "prx/libkernel/include/Api.hpp"
#endif

namespace
{
#if ANYUNREAL_TEST_CREATE_SHADER
    bool APS5_VABI exhausted(CommandBuffer*, std::uint32_t, void*)
    {
        throw std::length_error("AGC test command buffer exhausted");
    }

    void checkCommands(const AnyUnreal::RHI::AgcShader& shader, const AnyUnreal::Shader::UserDataLayout& layout, const AnyUnreal::Shader::UserDataInputs& inputs)
    {
        std::array<std::uint32_t, 256> words{};
        CommandBuffer commands{words.data(), words.data() + words.size(), words.data(), words.data() + words.size(), exhausted, nullptr, 0};
        AnyUnreal::RHI::WriteShaderRegisters(commands, shader, layout, inputs);
        const auto size = static_cast<std::size_t>(commands.cursor_up - words.data());
        std::map<std::uint32_t, std::uint32_t> registers;
        for (std::size_t offset = 0; offset < size;)
        {
            const auto header = words[offset];
            const auto count = static_cast<std::size_t>((header >> 16) & 0x3fff) + 2;
            if ((header >> 30) != 3 || ((header >> 8) & 255) != 0x76 || count < 3 || count > size - offset) throw std::runtime_error("Invalid AGC shader register packet");
            const auto base = words[offset + 1];
            for (std::size_t index = 2; index < count; ++index) registers[base + static_cast<std::uint32_t>(index - 2)] = words[offset + index];
            offset += count;
        }
        for (unsigned index = 0; index < shader.Get().num_sh_registers; ++index)
        {
            const auto& expected = shader.Get().sh_registers[index];
            if (registers.at(expected.offset) != expected.value) throw std::runtime_error("AGC program register was not written");
        }
        using AnyUnreal::Shader::ShaderStage;
        const auto base = layout.GetStage() == ShaderStage::Compute ? 0x240u : (layout.GetStage() == ShaderStage::Vertex || layout.GetStage() == ShaderStage::Geometry) ? 0x8cu : 0xcu;
        const auto expected = layout.Build(inputs);
        for (unsigned index = 0; index < layout.GetCount(); ++index)
        {
            if (registers.at(base + index) != expected[index]) throw std::runtime_error("AGC user data register was not written");
        }
        commands = {words.data(), words.data() + 1, words.data(), words.data() + 1, exhausted, nullptr, 0};
        try
        {
            AnyUnreal::RHI::WriteShaderRegisters(commands, shader, layout, inputs);
        }
        catch (const std::length_error&)
        {
            return;
        }
        throw std::runtime_error("AGC command buffer overflow was accepted");
    }
#endif
    std::vector<std::uint8_t> read(const char* path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file) throw std::runtime_error("Cannot open shader input");
        std::vector<std::uint8_t> bytes(std::istreambuf_iterator<char>(file), {});
        if (file.bad() || bytes.empty()) throw std::runtime_error("Cannot read shader input");
        return bytes;
    }

    template<class TCallable>
    void reject(TCallable callable)
    {
        try { callable(); }
        catch (const std::invalid_argument&) { return; }
        throw std::runtime_error("Invalid AGC header was accepted");
    }
}

int RunChecks(int argc, const char* const* argv)
{
    if (argc != 5 && argc != 8) throw std::invalid_argument("Expected header, code, stage and binding metadata, optionally followed by pixel header, code and metadata");
    const auto header = read(argv[1]);
    const auto code = read(argv[2]);
    const std::string_view name(argv[3]);
    using AnyUnreal::Shader::ShaderStage;
    ShaderStage stage;
    if (name == "vertex") stage = ShaderStage::Vertex;
    else if (name == "fragment") stage = ShaderStage::Pixel;
    else if (name == "compute") stage = ShaderStage::Compute;
    else if (name == "geometry") stage = ShaderStage::Geometry;
    else throw std::invalid_argument("Unknown shader stage");
    const auto metadata = read(argv[4]);
    const auto layout = AnyUnreal::Shader::UserDataLayout::Read(metadata, stage);
    AnyUnreal::Shader::UserDataInputs inputs;
    const auto aperture = static_cast<std::uint64_t>(layout.GetAddressHigh()) << 32;
    inputs.DescriptorSets = {aperture + 0x10000, aperture + 0x20000, aperture + 0x30000, aperture + 0x40000};
    inputs.VertexBuffers = aperture + 0x50000;
    inputs.PushConstants = aperture + 0x60000;
    inputs.RingTable = 0x800070000;
    inputs.BaseVertex = 17;
    const auto registers = layout.Build(inputs);
    for (std::size_t index = layout.GetCount(); index < registers.size(); ++index)
    {
        if (registers[index] != 0) throw std::runtime_error("AGC user data exceeds its declared window");
    }
    if (stage == ShaderStage::Vertex && layout.GetVertexBufferUsageMask() == 0 && (registers[0] != 17 || registers[1] != 0)) throw std::runtime_error("Unexpected fullscreen vertex draw parameters");
    if (stage == ShaderStage::Compute && registers[2] != 0x10000) throw std::runtime_error("Unexpected compute descriptor pointer");
    reject([&] { AnyUnreal::Shader::UserDataLayout::Read({}, stage); });
    auto duplicateMetadata = metadata;
    const std::string duplicate = "version=21\n";
    duplicateMetadata.insert(duplicateMetadata.end(), duplicate.begin(), duplicate.end());
    reject([&] { AnyUnreal::Shader::UserDataLayout::Read(duplicateMetadata, stage); });
    const auto rejectField = [&](const std::string& name, const std::string& value)
    {
        std::string text(metadata.begin(), metadata.end());
        const auto begin = text.find("\n" + name + "=");
        if (begin == std::string::npos) throw std::runtime_error("Missing metadata test field");
        const auto end = text.find('\n', begin + 1);
        text.replace(begin + 1, end - begin - 1, name + "=" + value);
        const auto bytes = std::span(reinterpret_cast<const std::uint8_t*>(text.data()), text.size());
        reject([&] { AnyUnreal::Shader::UserDataLayout::Read(bytes, stage); });
    };
    rejectField("version", "0");
    rejectField("user_sgpr_count", "17");
    rejectField("push_constants_valid", "2");
    rejectField("scratch_bytes_per_wave", "1");
    rejectField("vertex_buffer_per_attribute", "2");
    if (layout.GetVertexBufferUsageMask() == 0)
    {
        if (layout.UsesVertexBuffersPerAttribute()) throw std::runtime_error("Absent vertex buffers have an attribute layout");
        rejectField("vertex_buffer_usage_mask", "1");
        rejectField("vertex_buffer_per_attribute", "1");
    }
    if (stage == ShaderStage::Geometry)
    {
        rejectField("merged_geometry", "0");
        rejectField("merged_es_source_stage", "0");
        rejectField("esgs_system_sgprs_valid", "0");
        rejectField("merged_es_itemsize", "0");
        rejectField("merged_esgs_ring_itemsize", "0");
    }
    if (stage == ShaderStage::Vertex) rejectField("ngg_lds_layout_user_data_dword", "0");
    if (stage == ShaderStage::Compute)
    {
        auto invalid = inputs;
        invalid.DescriptorSets[0] = 0;
        reject([&] { layout.Build(invalid); });
        invalid.DescriptorSets[0] = ((static_cast<std::uint64_t>(layout.GetAddressHigh()) + 1) << 32) | 0x10000;
        reject([&] { layout.Build(invalid); });
    }
    using AnyUnreal::RHI::ValidateAgcHeader;
    ValidateAgcHeader(header, code, stage);
    for (std::size_t size = 0; size < header.size(); ++size) reject([&] { ValidateAgcHeader(std::span(header).first(size), code, stage); });
    reject([&] { ValidateAgcHeader(header, {}, stage); });
    reject([&] { ValidateAgcHeader(header, code, static_cast<ShaderStage>(99)); });
    for (auto field : {offsetof(Shader, user_data), offsetof(Shader, cx_registers), offsetof(Shader, sh_registers), offsetof(Shader, specials), offsetof(Shader, input_semantics), offsetof(Shader, output_semantics)})
    {
        auto invalid = header;
        std::fill_n(invalid.begin() + field, 8, 0);
        invalid[field + 3] = 1;
        reject([&] { ValidateAgcHeader(invalid, code, stage); });
    }
#if ANYUNREAL_TEST_CREATE_SHADER
    AnyUnreal::Kernel::CheckResult(sceAgcInit(11), "sceAgcInit");
    constexpr auto target = AnyUnreal::Shader::AgcTarget::ANYUNREAL_TEST_AGC_TARGET;
    const AnyUnreal::Shader::ShaderBinaryView binary{stage, target, header, code, metadata, {}, {}};
    reject([&] { AnyUnreal::RHI::AgcShader shader(binary, static_cast<ShaderStage>(99), target); });
    reject([&] { AnyUnreal::RHI::AgcShader shader(binary, stage, static_cast<AnyUnreal::Shader::AgcTarget>(99)); });
    AnyUnreal::RHI::AgcShader shader(binary, stage, target);
    if (!shader.Get().code || shader.Get().shader_size != code.size()) throw std::runtime_error("AGC shader creation failed");
    checkCommands(shader, layout, inputs);
    if (argc == 8)
    {
        if (stage != ShaderStage::Vertex && stage != ShaderStage::Geometry) throw std::invalid_argument("Graphics link check requires a vertex shader");
        const auto pixelHeader = read(argv[5]);
        const auto pixelCode = read(argv[6]);
        const auto pixelMetadata = read(argv[7]);
        const AnyUnreal::Shader::ShaderBinaryView pixelBinary{ShaderStage::Pixel, target, pixelHeader, pixelCode, pixelMetadata, {}, {}};
        const AnyUnreal::RHI::AgcShader pixel(pixelBinary, ShaderStage::Pixel, target);
        reject([&] { AnyUnreal::RHI::GraphicsShaderLink invalid(pixel, &shader, 4); });
        const AnyUnreal::RHI::GraphicsShaderLink link(shader, &pixel, 4);
        std::array<std::uint32_t, 256> words{};
        CommandBuffer commands{words.data(), words.data() + words.size(), words.data(), words.data() + words.size(), exhausted, nullptr, 0};
        link.Record(commands);
        const auto size = static_cast<std::size_t>(commands.cursor_up - words.data());
        std::size_t contextCount = 0;
        std::size_t primitiveCount = 0;
        for (std::size_t offset = 0; offset < size;)
        {
            const auto packet = words[offset];
            const auto count = static_cast<std::size_t>((packet >> 16) & 0x3fff) + 2;
            const auto opcode = (packet >> 8) & 255;
            if ((packet >> 30) != 3 || count < 3 || count > size - offset) throw std::runtime_error("Invalid graphics link register packet");
            if (opcode == 0x69) contextCount += count - 2;
            else if (opcode == 0x79) primitiveCount += count - 2;
            else throw std::runtime_error("Graphics link emitted an indirect or unexpected packet");
            offset += count;
        }
        if (contextCount != 34 || primitiveCount != 3) throw std::runtime_error("Graphics link register count is incomplete");
    }
#endif
    std::cout << "AGC header and shader checks passed\n";
    return 0;
}

#if ANYUNREAL_TEST_CREATE_SHADER
int main()
{
    return RunChecks(getargc_nid_postfix(), getargv_nid_postfix());
}
#else
int main(int argc, char** argv)
{
    return RunChecks(argc, argv);
}
#endif
