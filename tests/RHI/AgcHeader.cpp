#include "AnyUnreal/RHI/Shader/AgcHeader.h"
#include "AnyUnreal/Shader/UserData.h"
#include "SceShaders.hpp"
#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string_view>

#if ANYUNREAL_TEST_CREATE_SHADER
#include "AnyUnreal/RHI/Shader/AgcShader.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libSceAgc/libSceAgc.hpp"
#include "prx/libkernel/include/Api.hpp"
#endif

namespace
{
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
    if (argc != 5) throw std::invalid_argument("Expected header, code, stage and binding metadata");
    const auto header = read(argv[1]);
    const auto code = read(argv[2]);
    const std::string_view name(argv[3]);
    using AnyUnreal::Shader::ShaderStage;
    ShaderStage stage;
    if (name == "vertex") stage = ShaderStage::Vertex;
    else if (name == "fragment") stage = ShaderStage::Pixel;
    else if (name == "compute") stage = ShaderStage::Compute;
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
    if (stage == ShaderStage::Vertex && (registers[0] != 17 || registers[1] != 0)) throw std::runtime_error("Unexpected fullscreen vertex draw parameters");
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
    rejectField("version", "22");
    rejectField("user_sgpr_count", "17");
    rejectField("push_constants_valid", "2");
    rejectField("scratch_bytes_per_wave", "1");
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
