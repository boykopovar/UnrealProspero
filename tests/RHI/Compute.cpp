#include "AnyUnreal/RHI/Memory/BufferAllocation.h"
#include "AnyUnreal/RHI/Descriptor/Tables.h"
#include "AnyUnreal/Shader/ConstantData.h"
#include <algorithm>
#include <string_view>
#include "AnyUnreal/RHI/Command/Submission.h"
#include "AnyUnreal/RHI/Command/ComputeDispatch.h"
#include "AnyUnreal/RHI/Command/MemoryAcquire.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libSceAgc/libSceAgc.hpp"
#include "prx/libkernel/include/Api.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <limits>
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

    void checkDescriptorTables(std::uint32_t addressHigh, const void* buffer)
    {
        namespace RHI = AnyUnreal::RHI;
        namespace Shaders = AnyUnreal::Shader;
        using Kind = Shaders::ResourceKind;
        const std::array bindings{
            Shaders::RuntimeBinding{Kind::ShaderResource, 5, 2, 9, 2, 32, 16, 0, "storage-buffer"},
            Shaders::RuntimeBinding{Kind::UniformBuffer, 7, 3, 1, 1, 0, 16, 64, "uniform-buffer"}
        };
        RHI::DescriptorTables tables(bindings, addressHigh);
        const auto reject = [](auto&& action)
        {
            try { action(); }
            catch (const std::exception&) { return; }
            throw std::runtime_error("Invalid descriptor binding was accepted");
        };
        reject([&] { tables.SetBuffer(Kind::ShaderResource, 4, buffer, 64); });
        reject([&] { tables.SetBuffer(Kind::ShaderResource, 7, buffer, 64); });
        reject([&] { tables.SetBuffer(Kind::UniformBuffer, 7, buffer, 32); });
        const std::array<std::byte, 8> shortDescriptor{};
        reject([&] { tables.SetDescriptor(Kind::ShaderResource, 5, shortDescriptor); });
        tables.SetBuffer(Kind::ShaderResource, 5, buffer, 64);
        tables.SetBuffer(Kind::UniformBuffer, 7, buffer, 64);
        Shaders::UserDataInputs inputs;
        inputs.DescriptorSets.fill(123);
        reject([&] { tables.Apply(inputs); });
        for (const auto address : inputs.DescriptorSets)
        {
            if (address != 123) throw std::runtime_error("Incomplete descriptor application changed user data");
        }
        tables.SetBuffer(Kind::ShaderResource, 6, buffer, 128);
        tables.Apply(inputs);
        const auto ranges = tables.GetMemoryRanges();
        if (ranges.size() != 2 || reinterpret_cast<std::uintptr_t>(ranges[0].Address) != inputs.DescriptorSets[2] || ranges[0].Size != 64 || reinterpret_cast<std::uintptr_t>(ranges[1].Address) != inputs.DescriptorSets[3] || ranges[1].Size != 16) throw std::runtime_error("Descriptor memory ranges do not cover their tables");
        if (inputs.DescriptorSets[0] != 0 || inputs.DescriptorSets[1] != 0 || inputs.DescriptorSets[2] == 0 || inputs.DescriptorSets[3] == 0) throw std::runtime_error("Descriptor sets were assigned incorrectly");
        const auto* array = reinterpret_cast<const std::uint32_t*>(inputs.DescriptorSets[2]);
        const auto* uniform = reinterpret_cast<const std::uint32_t*>(inputs.DescriptorSets[3]);
        if (array[10] != 64 || array[14] != 128 || uniform[2] != 64) throw std::runtime_error("Descriptor array offsets were assigned incorrectly");
        for (unsigned index = 0; index < 8; ++index)
        {
            if (array[index] != 0) throw std::runtime_error("Descriptor write overwrote layout padding");
        }
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
    if (operation != "write" && operation != "read-write" && operation != "constants") throw std::invalid_argument("Unknown compute operation");
    const bool readWrite = operation == "read-write";
    const bool withConstants = operation == "constants";
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
    checkDescriptorTables(layout.GetAddressHigh(), secondMemory.Data());
    const std::array<std::byte, 4> tail{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}};
    secondMemory.Upload(secondMemory.Size() - tail.size(), tail);
    const auto storedTail = secondMemory.GetRange(secondMemory.Size() - tail.size(), tail.size());
    if (!std::equal(tail.begin(), tail.end(), storedTail.begin())) throw std::runtime_error("Buffer tail upload failed");
    bool rangeRejected = false;
    try { secondMemory.Upload(secondMemory.Size() - 3, tail); }
    catch (const std::out_of_range&) { rangeRejected = true; }
    if (!rangeRejected || !std::equal(tail.begin(), tail.end(), storedTail.begin())) throw std::runtime_error("Out-of-range upload modified the buffer");
    auto* base = static_cast<std::byte*>(memory.Data());
    auto* fence = std::construct_at(reinterpret_cast<std::uint64_t*>(base + 64), 0);
    auto* output = reinterpret_cast<std::uint32_t*>(base + 256);
    constexpr std::size_t inputOffset = 256;
    auto* input = reinterpret_cast<std::uint32_t*>(secondMemory.GetRange(inputOffset, 64 * sizeof(std::uint32_t)).data());
    auto* words = reinterpret_cast<std::uint32_t*>(base + 4096);
    std::vector<Shaders::RuntimeBinding> bindings;
    if (readWrite) bindings.push_back({Shaders::ResourceKind::ShaderResource, 0, 0, 0, 1, 0, 16, 0, "storage-buffer"});
    bindings.push_back({Shaders::ResourceKind::UnorderedAccess, 0, 0, readWrite ? 1u : 0u, 1, readWrite ? 16u : 0u, 16, 0, "storage-buffer"});
    if (withConstants) bindings.push_back({Shaders::ResourceKind::UniformBuffer, 0, 1, 0, 1, 0, 16, 16, "uniform-buffer"});
    Shaders::ConstantData constantValues(bindings);
    RHI::DescriptorTables descriptors(bindings, layout.GetAddressHigh());
    Shaders::UserDataInputs inputs;
    bool rejected = false;
    try { descriptors.Apply(inputs); }
    catch (const std::runtime_error&) { rejected = true; }
    if (!rejected) throw std::runtime_error("Uninitialized descriptor table was accepted");
    descriptors.SetBuffer(Shaders::ResourceKind::UnorderedAccess, 0, output, 64 * sizeof(std::uint32_t));
    if (readWrite) descriptors.SetBuffer(Shaders::ResourceKind::ShaderResource, 0, input, 64 * sizeof(std::uint32_t));
    if (withConstants) descriptors.SetBuffer(Shaders::ResourceKind::UniformBuffer, 0, memory.GetRange(1024, 16).data(), 16);
    descriptors.Apply(inputs);
    for (std::uint64_t iteration = 1; iteration <= 3; ++iteration)
    {
        const std::array<std::uint32_t, 4> constants{static_cast<std::uint32_t>(iteration + 2), static_cast<std::uint32_t>(iteration * 11), 0, 0};
        if (withConstants)
        {
            const auto source = std::as_bytes(std::span(constants));
            constantValues.Write(0, 0, source, 0, 4);
            constantValues.Write(0, 4, source, 4, 4);
            memory.Upload(1024, constantValues.GetBlocks().at(0).Data);
        }
        std::array<std::uint32_t, 64> values{};
        for (unsigned index = 0; index < values.size(); ++index)
        {
            output[index] = 0xffffffff;
            values[index] = index + static_cast<std::uint32_t>(iteration * 7);
        }
        secondMemory.Upload(inputOffset, std::as_bytes(std::span(values)));
        const std::array<std::uint32_t, 3> dispatchGroups{iteration == 3 ? 0u : 1u, 1u, 1u};
        memory.Upload(2048, std::as_bytes(std::span(dispatchGroups)));
        CommandBuffer commands{words, words + 1024, words, words + 1024, exhausted, nullptr, 0};
        sceAgcDcbContextStateOp(&commands, 0);
        std::vector<RHI::GpuMemoryRange> ranges{
            {memory.Data(), memory.Size()},
            {secondMemory.Data(), secondMemory.Size()},
            {shader.Get().code, shader.Get().shader_size}
        };
        const auto originalPosition = commands.cursor_up;
        for (const auto offset : {2u, static_cast<unsigned>(memory.Size() - 8)})
        {
            bool rejectedArguments = false;
            try { RHI::WriteIndirectComputeDispatch(commands, shader, layout, inputs, memory.Data(), memory.Size(), offset); }
            catch (const std::out_of_range&) { rejectedArguments = true; }
            if (!rejectedArguments || commands.cursor_up != originalPosition) throw std::runtime_error("Invalid indirect dispatch changed the command buffer");
        }
        const RHI::GpuMemoryRange invalidRanges[]{
            {nullptr, 4},
            {memory.Data(), 0},
            {reinterpret_cast<const void*>(std::numeric_limits<std::uintptr_t>::max()), 2}
        };
        for (const auto& invalidRange : invalidRanges)
        {
            const RHI::GpuMemoryRange batch[]{ranges.front(), invalidRange};
            bool rejected = false;
            try { RHI::WriteMemoryAcquire(commands, batch); }
            catch (const std::invalid_argument&) { rejected = true; }
            if (!rejected || commands.cursor_up != originalPosition) throw std::runtime_error("Invalid acquire range changed the command buffer");
        }
        const auto tableRanges = descriptors.GetMemoryRanges();
        ranges.insert(ranges.end(), tableRanges.begin(), tableRanges.end());
        RHI::WriteMemoryAcquire(commands, ranges);
        if (iteration == 1) RHI::WriteComputeDispatch(commands, shader, layout, inputs, 1, 1, 1);
        else RHI::WriteIndirectComputeDispatch(commands, shader, layout, inputs, memory.Data(), memory.Size(), 2048);
        RHI::SubmitAndWait(commands, *fence, iteration, 10000000);
        for (unsigned index = 0; index < 64; ++index)
        {
            const auto expected = withConstants ? index * constants[0] + constants[1] : (readWrite ? input[index] : index) * 3 + 1;
            if (output[index] != (iteration == 3 ? 0xffffffff : expected)) throw std::runtime_error("GPU compute output does not match the shader");
        }
    }
    std::cout << "GPU compute output verified for direct, indirect and zero-sized indirect dispatches\n";
}
