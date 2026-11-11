#include "AnyUnreal/RHI/Memory/BufferAllocation.h"
#include "AnyUnreal/RHI/Descriptor/Buffer.h"
#include <algorithm>
#include <string_view>
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
    if (getargc_nid_postfix() != 5) throw std::invalid_argument("Expected compute header, code, metadata and operation");
    const auto* arguments = getargv_nid_postfix();
    const std::string_view operation(arguments[4]);
    if (operation != "write" && operation != "read-write") throw std::invalid_argument("Unknown compute operation");
    const bool readWrite = operation == "read-write";
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
    auto* input = static_cast<std::uint32_t*>(secondMemory.Data());
    auto* words = reinterpret_cast<std::uint32_t*>(base + 4096);
    const auto outputDescriptor = RHI::CreateByteAddressBufferDescriptor(output, 64 * sizeof(std::uint32_t));
    std::copy(outputDescriptor.begin(), outputDescriptor.end(), descriptor + (readWrite ? 4 : 0));
    if (readWrite)
    {
        const auto inputDescriptor = RHI::CreateByteAddressBufferDescriptor(input, 64 * sizeof(std::uint32_t));
        std::copy(inputDescriptor.begin(), inputDescriptor.end(), descriptor);
    }
    Shaders::UserDataInputs inputs;
    inputs.DescriptorSets[0] = reinterpret_cast<std::uintptr_t>(descriptor);
    for (std::uint64_t iteration = 1; iteration <= 2; ++iteration)
    {
        for (unsigned index = 0; index < 64; ++index)
        {
            output[index] = 0xffffffff;
            input[index] = index + static_cast<std::uint32_t>(iteration * 7);
        }
        CommandBuffer commands{words, words + 1024, words, words + 1024, exhausted, nullptr, 0};
        sceAgcDcbContextStateOp(&commands, 0);
        sceAgcDcbAcquireMem(&commands, 1, 0, 0x4381, memory.Data(), memory.Size(), 40);
        sceAgcDcbAcquireMem(&commands, 1, 0, 0x4381, secondMemory.Data(), secondMemory.Size(), 40);
        sceAgcDcbAcquireMem(&commands, 1, 0, 0x4381, shader.Get().code, shader.Get().shader_size, 40);
        RHI::WriteShaderRegisters(commands, shader, layout, inputs);
        sceAgcCbDispatch(&commands, 1, 1, 1, shader.Get().specials->dispatch_modifier);
        RHI::SubmitAndWait(commands, *fence, iteration, 10000000);
        for (unsigned index = 0; index < 64; ++index)
        {
            if (output[index] != (readWrite ? input[index] : index) * 3 + 1) throw std::runtime_error("GPU compute output does not match the shader");
        }
    }
    std::cout << "GPU compute output verified for two dispatches\n";
}
