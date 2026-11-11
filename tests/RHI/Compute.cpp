#include "AnyUnreal/RHI/Memory/BufferAllocation.h"
#include "AnyUnreal/RHI/Command/Submission.h"
#include "AnyUnreal/RHI/Shader/ShaderCommands.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libSceAgc/libSceAgc.hpp"
#include "prx/libkernel/include/Api.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>

namespace
{
    std::vector<std::uint8_t> read(const char* path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file) throw std::runtime_error("Cannot open compute shader input");
        std::vector<std::uint8_t> result(std::istreambuf_iterator<char>(file), {});
        if (file.bad() || result.empty()) throw std::runtime_error("Cannot read compute shader input");
        return result;
    }

    bool APS5_VABI exhausted(CommandBuffer*, std::uint32_t, void*)
    {
        throw std::length_error("Compute test command buffer exhausted");
    }
}

int main()
{
    if (getargc_nid_postfix() != 4) throw std::invalid_argument("Expected compute header, code and metadata");
    const auto* arguments = getargv_nid_postfix();
    const auto header = read(arguments[1]);
    const auto code = read(arguments[2]);
    const auto metadata = read(arguments[3]);
    namespace Kernel = AnyUnreal::Kernel;
    namespace RHI = AnyUnreal::RHI;
    namespace Shaders = AnyUnreal::Shader;
    constexpr auto stage = Shaders::ShaderStage::Compute;
    constexpr auto target = Shaders::AgcTarget::ANYUNREAL_TEST_AGC_TARGET;
    const auto layout = Shaders::UserDataLayout::Read(metadata, stage);
    Kernel::CheckResult(sceAgcInit(11), "sceAgcInit");
    const Shaders::ShaderBinaryView binary{stage, target, header, code, metadata, {}, {}};
    RHI::AgcShader shader(binary, stage, target);
    RHI::BufferAllocation memory(16384, layout.GetAddressHigh());
    RHI::BufferAllocation secondMemory(16384, layout.GetAddressHigh());
    if (secondMemory.Data() == memory.Data()) throw std::runtime_error("GPU allocations overlap");
    auto* base = static_cast<std::byte*>(memory.Data());
    auto* descriptor = reinterpret_cast<std::uint32_t*>(base);
    auto* fence = std::construct_at(reinterpret_cast<std::uint64_t*>(base + 64), 0);
    auto* output = reinterpret_cast<std::uint32_t*>(base + 256);
    auto* words = reinterpret_cast<std::uint32_t*>(base + 4096);
    const auto outputAddress = reinterpret_cast<std::uintptr_t>(output);
    descriptor[0] = static_cast<std::uint32_t>(outputAddress);
    descriptor[1] = static_cast<std::uint32_t>(outputAddress >> 32);
    descriptor[2] = 64 * sizeof(std::uint32_t);
    descriptor[3] = 0x31016fac;
    Shaders::UserDataInputs inputs;
    inputs.DescriptorSets[0] = reinterpret_cast<std::uintptr_t>(descriptor);
    for (std::uint64_t iteration = 1; iteration <= 2; ++iteration)
    {
        for (unsigned index = 0; index < 64; ++index) output[index] = 0xffffffff;
        CommandBuffer commands{words, words + 1024, words, words + 1024, exhausted, nullptr, 0};
        sceAgcDcbContextStateOp(&commands, 0);
        sceAgcDcbAcquireMem(&commands, 1, 0, 0x4381, memory.Data(), memory.Size(), 40);
        sceAgcDcbAcquireMem(&commands, 1, 0, 0x4381, shader.Get().code, shader.Get().shader_size, 40);
        RHI::WriteShaderRegisters(commands, shader, layout, inputs);
        sceAgcCbDispatch(&commands, 1, 1, 1, shader.Get().specials->dispatch_modifier);
        RHI::SubmitAndWait(commands, *fence, iteration, 10000000);
        for (unsigned index = 0; index < 64; ++index)
        {
            if (output[index] != index * 3 + 1) throw std::runtime_error("GPU compute output does not match the shader");
        }
    }
    std::cout << "GPU compute output verified for two dispatches\n";
}
