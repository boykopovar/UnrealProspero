#include "AnyUnreal/RHI/Shader/AgcHeader.h"
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
    if (argc != 4) throw std::invalid_argument("Expected header, code and stage");
    const auto header = read(argv[1]);
    const auto code = read(argv[2]);
    const std::string_view name(argv[3]);
    using AnyUnreal::Shader::ShaderStage;
    ShaderStage stage;
    if (name == "vertex") stage = ShaderStage::Vertex;
    else if (name == "fragment") stage = ShaderStage::Pixel;
    else if (name == "compute") stage = ShaderStage::Compute;
    else throw std::invalid_argument("Unknown shader stage");
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
    const AnyUnreal::Shader::ShaderBinaryView binary{stage, target, header, code, {}, {}, {}};
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
