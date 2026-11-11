#include "AnyUnreal/RHI/Shader/GraphicsShaderLink.h"
#include "AnyUnreal/RHI/Shader/ShaderCommands.h"
#include "AnyUnreal/RHI/Command/CommandStorage.h"
#include "AnyUnreal/RHI/Command/MemoryAcquire.h"
#include "AnyUnreal/RHI/Command/Submission.h"
#include "AnyUnreal/RHI/Command/Viewport.h"
#include "AnyUnreal/RHI/Descriptor/BufferDescriptor.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libSceAgc/libSceAgc.hpp"
#include "prx/libkernel/include/Api.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>

namespace
{
    std::vector<std::uint8_t> read(const char* path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file) throw std::runtime_error("Cannot open graphics shader input");
        std::vector<std::uint8_t> bytes(std::istreambuf_iterator<char>(file), {});
        if (file.bad() || bytes.empty()) throw std::runtime_error("Cannot read graphics shader input");
        return bytes;
    }
}

int main()
{
    if (getargc_nid_postfix() != 7) throw std::invalid_argument("Expected vertex and pixel header, code and metadata");
    const auto* argv = getargv_nid_postfix();
    using namespace AnyUnreal::RHI;
    namespace Shaders = AnyUnreal::Shader;
    const auto vertexHeader = read(argv[1]);
    const auto vertexCode = read(argv[2]);
    const auto vertexMetadata = read(argv[3]);
    const auto pixelHeader = read(argv[4]);
    const auto pixelCode = read(argv[5]);
    const auto pixelMetadata = read(argv[6]);
    const auto vertexLayout = Shaders::UserDataLayout::Read(vertexMetadata, Shaders::ShaderStage::Vertex);
    const auto pixelLayout = Shaders::UserDataLayout::Read(pixelMetadata, Shaders::ShaderStage::Pixel);
    if (vertexLayout.GetVertexBufferUsageMask() != 0x88 || vertexLayout.UsesVertexBuffersPerAttribute()) throw std::invalid_argument("Graphics fixture requires vertex bindings three and seven");
    const auto aperture = vertexLayout.GetAddressHigh();
    if (pixelLayout.GetAddressHigh() != aperture) throw std::invalid_argument("Shader apertures disagree");
    AnyUnreal::Kernel::CheckResult(sceAgcInit(11), "sceAgcInit");
    constexpr auto target = Shaders::AgcTarget::ANYUNREAL_TEST_AGC_TARGET;
    const AgcShader vertex({Shaders::ShaderStage::Vertex, target, vertexHeader, vertexCode, vertexMetadata, {}, {}}, Shaders::ShaderStage::Vertex, target);
    const AgcShader pixel({Shaders::ShaderStage::Pixel, target, pixelHeader, pixelCode, pixelMetadata, {}, {}}, Shaders::ShaderStage::Pixel, target);
    const GraphicsShaderLink link(vertex, &pixel, 4);
    BufferAllocation output(64 * 64 * 4, aperture);
    BufferAllocation positions(36, aperture);
    BufferAllocation colors(48, aperture);
    BufferAllocation descriptors(32, aperture);
    BufferAllocation fenceStorage(8, aperture);
    std::fill_n(static_cast<std::uint32_t*>(output.Data()), 64 * 64, 0);
    const std::array<float, 9> positionValues{-1, -1, 0, 3, -1, 0, -1, 3, 0};
    const std::array<float, 12> colorValues{0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1};
    positions.Upload(0, std::as_bytes(std::span(positionValues)));
    colors.Upload(0, std::as_bytes(std::span(colorValues)));
    const std::array tables{CreateVertexBufferDescriptor(positions.Data(), positions.Size(), 12), CreateVertexBufferDescriptor(colors.Data(), colors.Size(), 16)};
    descriptors.Upload(0, std::as_bytes(std::span(tables)));
    auto& fence = *std::construct_at(static_cast<std::uint64_t*>(fenceStorage.Data()), 0);
    CommandStorage storage(16384, aperture);
    auto& commands = storage.Get();
    sceAgcDcbContextStateOp(&commands, 0);
    const std::array ranges{GpuMemoryRange{output.Data(), output.Size()}, GpuMemoryRange{positions.Data(), positions.Size()}, GpuMemoryRange{colors.Data(), colors.Size()}, GpuMemoryRange{descriptors.Data(), descriptors.Size()}, GpuMemoryRange{vertex.Get().code, vertex.Get().shader_size}, GpuMemoryRange{pixel.Get().code, pixel.Get().shader_size}};
    WriteMemoryAcquire(commands, ranges);
    link.Record(commands);
    for (const auto* shader : {&vertex.Get(), &pixel.Get()})
    {
        for (std::uint32_t index = 0; index < shader->num_cx_registers; ++index) sceAgcDcbSetCxRegisterDirect(&commands, shader->cx_registers[index]);
    }
    const auto address = reinterpret_cast<std::uintptr_t>(output.Data());
    const std::array state{
        ShaderRegister{0x318, static_cast<std::uint32_t>(address >> 8)}, ShaderRegister{0x31b, 0}, ShaderRegister{0x31c, 0x8028},
        ShaderRegister{0x31d, 0}, ShaderRegister{0x31e, 0}, ShaderRegister{0x31f, 0}, ShaderRegister{0x321, 0},
        ShaderRegister{0x323, 0}, ShaderRegister{0x324, 0}, ShaderRegister{0x325, 0},
        ShaderRegister{0x390, static_cast<std::uint32_t>(address >> 40)}, ShaderRegister{0x398, 0}, ShaderRegister{0x3a0, 0}, ShaderRegister{0x3a8, 0},
        ShaderRegister{0x3b0, 63 | (63 << 14)}, ShaderRegister{0x3b8, 0x4d000000},
        ShaderRegister{0xc, 0}, ShaderRegister{0xd, 64 | (64 << 16)}, ShaderRegister{0x8e, 15}, ShaderRegister{0x200, 0},
        ShaderRegister{0x202, 0xcc0010}, ShaderRegister{0x205, 0x240}, ShaderRegister{0x206, 0x43f}, ShaderRegister{0x204, 0x1080000},
        ShaderRegister{0x292, 0x22}, ShaderRegister{0x2f9, 0x2d}, ShaderRegister{0x1e0, 0},
        ShaderRegister{0x2fa, 0x3f800000}, ShaderRegister{0x2fb, 0x3f800000}, ShaderRegister{0x2fc, 0x3f800000}, ShaderRegister{0x2fd, 0x3f800000}
    };
    for (const auto value : state) sceAgcDcbSetCxRegisterDirect(&commands, value);
    WriteViewport(commands, 0, {0, 0, 0, 64, 64, 1});
    WriteViewportScissor(commands, 0, {0, 0, 64, 64});
    WriteScissor(commands, {8, 8, 56, 56});
    Shaders::UserDataInputs vertexInputs;
    vertexInputs.VertexBuffers = reinterpret_cast<std::uintptr_t>(descriptors.Data());
    WriteShaderRegisters(commands, vertex, vertexLayout, vertexInputs);
    WriteShaderRegisters(commands, pixel, pixelLayout, {});
    sceAgcDcbSetNumInstances(&commands, 1);
    sceAgcDcbDrawIndexAuto(&commands, 3, 0);
    SubmitAndWait(commands, fence, 1, 10000000);
    const auto* pixels = static_cast<const std::uint32_t*>(output.Data());
    for (unsigned y = 0; y < 64; ++y)
    {
        for (unsigned x = 0; x < 64; ++x)
        {
            const auto expected = x >= 8 && x < 56 && y >= 8 && y < 56 ? 0xff00ff00u : 0u;
            if (pixels[y * 64 + x] != expected) throw std::runtime_error("Graphics pixel mismatch at " + std::to_string(x) + "," + std::to_string(y) + ": " + std::to_string(pixels[y * 64 + x]));
        }
    }
    std::cout << "Vertex buffer draw and scissor pixels verified\n";
    return 0;
}
