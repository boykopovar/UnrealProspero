#include "AnyUnreal/RHI/Shader/GraphicsShaderLink.h"
#include "AnyUnreal/RHI/Shader/ShaderCommands.h"
#include "AnyUnreal/RHI/Command/CommandStorage.h"
#include "AnyUnreal/RHI/Command/MemoryAcquire.h"
#include "AnyUnreal/RHI/Command/Submission.h"
#include "AnyUnreal/RHI/Command/Viewport.h"
#include "AnyUnreal/RHI/Command/ColorTarget.h"
#include "AnyUnreal/RHI/Command/ColorClear.h"
#if ANYUNREAL_TEST_DEPTH
#include "AnyUnreal/RHI/Command/DepthTarget.h"
#include "AnyUnreal/RHI/Command/DepthBias.h"
#include "AnyUnreal/RHI/Command/MemoryBarrier.h"
#endif
#if ANYUNREAL_TEST_OCCLUSION
#include "AnyUnreal/RHI/Query/OcclusionSnapshot.h"
#endif
#include "AnyUnreal/RHI/Command/IndirectDraw.h"
#include "AnyUnreal/RHI/Descriptor/VertexBufferTable.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libSceAgc/libSceAgc.hpp"
#include "prx/libSceAgc/DcbDraw/include/IndexBuffer.hpp"
#include "prx/libkernel/include/Api.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

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
    if (getargc_nid_postfix() != 7 && getargc_nid_postfix() != 8) throw std::invalid_argument("Expected vertex and pixel header, code and metadata, optionally followed by geometry");
    const auto* argv = getargv_nid_postfix();
    using namespace AnyUnreal::RHI;
    namespace Shaders = AnyUnreal::Shader;
    const bool geometry = getargc_nid_postfix() == 8;
    if (geometry && std::string_view(argv[7]) != "geometry") throw std::invalid_argument("Unknown graphics program stage");
    const auto vertexStage = geometry ? Shaders::ShaderStage::Geometry : Shaders::ShaderStage::Vertex;
    const auto vertexHeader = read(argv[1]);
    const auto vertexCode = read(argv[2]);
    const auto vertexMetadata = read(argv[3]);
    const auto pixelHeader = read(argv[4]);
    const auto pixelCode = read(argv[5]);
    const auto pixelMetadata = read(argv[6]);
    const auto vertexLayout = Shaders::UserDataLayout::Read(vertexMetadata, vertexStage);
    const auto pixelLayout = Shaders::UserDataLayout::Read(pixelMetadata, Shaders::ShaderStage::Pixel);
    const bool dynamic = vertexLayout.UsesDynamicVertexInput();
    if (vertexLayout.GetVertexBufferUsageMask() != (dynamic ? 3u : 0x88u) || vertexLayout.UsesVertexBuffersPerAttribute() != dynamic) throw std::invalid_argument("Graphics fixture has an unexpected vertex layout");
    const auto aperture = vertexLayout.GetAddressHigh();
    if (pixelLayout.GetAddressHigh() != aperture) throw std::invalid_argument("Shader apertures disagree");
    AnyUnreal::Kernel::CheckResult(sceAgcInit(11), "sceAgcInit");
    constexpr auto target = Shaders::AgcTarget::ANYUNREAL_TEST_AGC_TARGET;
    const AgcShader vertex({vertexStage, target, vertexHeader, vertexCode, vertexMetadata, {}, {}}, vertexStage, target);
    const AgcShader pixel({Shaders::ShaderStage::Pixel, target, pixelHeader, pixelCode, pixelMetadata, {}, {}}, Shaders::ShaderStage::Pixel, target);
    const GraphicsShaderLink link(vertex, &pixel, 4);
#if ANYUNREAL_TEST_DEPTH
    for (const auto dimensions : {std::array{0u, 1u}, std::array{1u, 0u}, std::array{16385u, 1u}, std::array{1u, 16385u}})
    {
        bool rejected = false;
        try { static_cast<void>(DepthTexture2D::CalculateDataSize(dimensions[0], dimensions[1])); }
        catch (const std::out_of_range&) { rejected = true; }
        if (!rejected) throw std::runtime_error("Invalid depth dimensions were accepted");
    }
    if (DepthTexture2D::CalculateDataSize(128, 128) != 65536 || DepthTexture2D::CalculateDataSize(129, 128) != 131072 || DepthTexture2D::CalculateDataSize(129, 129) != 262144) throw std::runtime_error("Depth tile allocation size is incorrect");
    for (const auto depthMode : {0u, 1u, 2u, 3u, 4u, 5u, 6u})
#endif
    for (unsigned vertexFormat = 0; vertexFormat < (dynamic ? 2u : 1u); ++vertexFormat)
    for (unsigned instanceMode = 0; instanceMode < (dynamic ? 4u : 1u); ++instanceMode)
    for (const auto indirect : {false, true})
    for (const auto indexStride : {0u, 2u, 4u})
    for (const auto baseVertex : {0u, 1u})
    for (const auto format : {TextureFormat::R8Unorm, TextureFormat::Rg8Unorm, TextureFormat::Rgba8Unorm, TextureFormat::Rgba8Srgb, TextureFormat::Bgra8Unorm, TextureFormat::Bgra8Srgb, TextureFormat::R32Float, TextureFormat::Rgba32Float, TextureFormat::Rgba16Float, TextureFormat::R16Float, TextureFormat::Rg16Float})
    for (const auto dimensions : {std::array{64u, 64u}, std::array{67u, 39u}, std::array{13u, 7u}, std::array{259u, 137u}})
    {
        const auto width = dimensions[0];
        const auto height = dimensions[1];
        std::string caseName = std::to_string(width) + "x" + std::to_string(height) + " indirect " + std::to_string(indirect) + " index stride " + std::to_string(indexStride) + " base vertex " + std::to_string(baseVertex);
#if ANYUNREAL_TEST_DEPTH
        caseName += " depth mode " + std::to_string(depthMode) + " barrier " + std::to_string(ANYUNREAL_TEST_DEPTH_BARRIER);
#endif
        LinearTexture2D targetTexture(width, height, format, aperture);
#if ANYUNREAL_TEST_DEPTH
        LinearTexture2D secondTarget(width, height, format, aperture);
#endif
        const auto& output = targetTexture.GetStorage();
        auto positions = std::make_shared<BufferAllocation>(52 + baseVertex * 12, aperture);
        auto colors = std::make_shared<BufferAllocation>(128, aperture);
        std::fill_n(static_cast<std::byte*>(colors->Data()), colors->Size(), std::byte{});
        const auto instanceCount = instanceMode ? 3u : 1u;
        const auto firstInstance = instanceMode ? 2u : 0u;
        const auto divisor = instanceMode == 2 ? 2u : instanceMode == 3 ? 0u : 1u;
        const auto lastColorIndex = firstInstance + (divisor == 0 ? 0u : (instanceCount - 1) / divisor);
        BufferAllocation fenceStorage(8, aperture);
        BufferAllocation indexStorage(16, aperture);
        BufferAllocation argumentStorage(32, aperture);
        if (indirect)
        {
            if (indexStride)
            {
                const std::array arguments{3u, instanceCount, 1u, baseVertex - 1, firstInstance};
                argumentStorage.Upload(4, std::as_bytes(std::span(arguments)));
            }
            else
            {
                const std::array arguments{3u, instanceCount, baseVertex, firstInstance};
                argumentStorage.Upload(4, std::as_bytes(std::span(arguments)));
            }
        }
        if (indexStride == 2)
        {
            const std::array<std::uint16_t, 4> indices{99, 1, 2, 3};
            indexStorage.Upload(0, std::as_bytes(std::span(indices)));
        }
        else if (indexStride == 4)
        {
            const std::array<std::uint32_t, 4> indices{99, 1, 2, 3};
            indexStorage.Upload(0, std::as_bytes(std::span(indices)));
        }
        std::fill_n(static_cast<std::uint32_t*>(output.Data()), output.Size() / 4, 0xa5a5a5a5u);
#if ANYUNREAL_TEST_DEPTH
        const float vertexDepth = depthMode >= 3 ? 0.5f : 0.0f;
#else
        constexpr float vertexDepth = 0;
#endif
        std::array<float, 9> positionValues{-1, -1, vertexDepth, 3, -1, vertexDepth, -1, 3, vertexDepth};
#if ANYUNREAL_TEST_DEPTH
        if (depthMode >= 5)
        {
            positionValues[2] = 0.25f;
            positionValues[5] = 0.75f;
            positionValues[8] = 0.25f;
        }
#endif
        const std::array<float, 12> colorValues{0.5f, 0.25f, 0.75f, 1, 0.5f, 0.25f, 0.75f, 1, 0.5f, 0.25f, 0.75f, 1};
        positions->Upload(16 + baseVertex * 12, std::as_bytes(std::span(positionValues)));
        const auto colorStride = vertexFormat == 0 ? 16u : 8u;
        const auto colorOffset = 32 + (instanceMode ? lastColorIndex : baseVertex) * colorStride;
        if (vertexFormat == 0) colors->Upload(colorOffset, std::as_bytes(std::span(colorValues).first(instanceMode ? 4 : 12)));
        else
        {
            const std::array<std::uint16_t, 12> halfColors{0x3800, 0x3400, 0x3a00, 0x3c00, 0x3800, 0x3400, 0x3a00, 0x3c00, 0x3800, 0x3400, 0x3a00, 0x3c00};
            colors->Upload(colorOffset, std::as_bytes(std::span(halfColors).first(instanceMode ? 4 : 12)));
        }
        const auto rejectTable = [&](std::uint32_t mask, std::span<const VertexBufferBinding> bindings)
        {
            try
            {
                const VertexBufferTable invalid(mask, bindings, aperture);
            }
            catch (const std::invalid_argument&)
            {
                return;
            }
            catch (const std::out_of_range&)
            {
                return;
            }
            throw std::runtime_error("Invalid vertex table was accepted");
        };
        rejectTable(0x88, std::array{VertexBufferBinding{3, positions, 16, 12}});
        rejectTable(0x88, std::array{VertexBufferBinding{3, positions, 16, 12}, VertexBufferBinding{3, colors, 32, 16}});
        rejectTable(8, std::array{VertexBufferBinding{32, positions, 16, 12}});
        rejectTable(8, std::array{VertexBufferBinding{3, {}, 0, 12}});
        rejectTable(8, std::array{VertexBufferBinding{3, positions, positions->Size(), 12}});
        rejectTable(8, std::array{VertexBufferBinding{3, positions, 16, 0x4000}});
        const VertexBufferTable emptyTable(0, {}, aperture);
        Shaders::UserDataInputs emptyInputs;
        emptyInputs.VertexBuffers = 1;
        emptyTable.Apply(emptyInputs);
        if (emptyInputs.VertexBuffers != 0 || !emptyTable.GetMemoryRanges().empty()) throw std::runtime_error("Empty vertex table retained GPU state");
        const std::weak_ptr<BufferAllocation> positionLifetime = positions;
        const std::weak_ptr<BufferAllocation> colorLifetime = colors;
        const auto colorFormat = !dynamic ? BufferFormat::Untyped : vertexFormat == 0 ? BufferFormat::Float4 : BufferFormat::Half4;
        const VertexBufferTable descriptors(vertexLayout.GetVertexBufferUsageMask(), std::array{VertexBufferBinding{dynamic ? 1u : 7u, colors, 32, colorStride, colorFormat, divisor, instanceMode != 0}, VertexBufferBinding{dynamic ? 0u : 3u, positions, 16, 12, dynamic ? BufferFormat::Float3 : BufferFormat::Untyped}}, aperture, dynamic);
        positions.reset();
        colors.reset();
        if (positionLifetime.expired() || colorLifetime.expired()) throw std::runtime_error("Vertex table did not retain source buffers");
        auto& fence = *std::construct_at(static_cast<std::uint64_t*>(fenceStorage.Data()), 0);
        CommandStorage storage(16384, aperture);
        auto& commands = storage.Get();
        sceAgcDcbContextStateOp(&commands, 0);
        WriteColorClear(commands, targetTexture, {});
#if ANYUNREAL_TEST_DEPTH
        WriteColorClear(commands, secondTarget, {});
        DepthTexture2D depthTexture(width, height, aperture);
#if ANYUNREAL_TEST_DEPTH_BARRIER
        MemoryBarrier depthBarrier(aperture);
#endif
        if (reinterpret_cast<std::uintptr_t>(depthTexture.GetStorage().Data()) % 65536 != 0 || depthTexture.GetStorage().Size() != DepthTexture2D::CalculateDataSize(width, height)) throw std::runtime_error("Depth texture allocation does not match its tiled layout");
        WriteDepthTarget(commands, depthTexture);
        WriteDepthClear(commands, depthTexture, depthMode == 0 ? 0.0f : 1.0f);
#endif
        const std::array ranges{GpuMemoryRange{output.Data(), output.Size()}, GpuMemoryRange{indexStorage.Data(), indexStorage.Size()}, GpuMemoryRange{vertex.Get().code, vertex.Get().shader_size}, GpuMemoryRange{pixel.Get().code, pixel.Get().shader_size}};
        WriteMemoryAcquire(commands, ranges);
        WriteMemoryAcquire(commands, descriptors.GetMemoryRanges());
        link.Record(commands);
        for (const auto* shader : {&vertex.Get(), &pixel.Get()})
        {
            for (std::uint32_t index = 0; index < shader->num_cx_registers; ++index) sceAgcDcbSetCxRegisterDirect(&commands, shader->cx_registers[index]);
        }
        WriteColorTarget(commands, 0, targetTexture);
        for (std::uint32_t index = 1; index < 8; ++index) DisableColorTarget(commands, index);
        const std::array state{
            ShaderRegister{0xc, 0}, ShaderRegister{0xd, width | (height << 16)}, ShaderRegister{0x8e, 15}, ShaderRegister{0x200, 0},
            ShaderRegister{0x202, 0xcc0010}, ShaderRegister{0x205, 0x240}, ShaderRegister{0x206, 0x43f}, ShaderRegister{0x204, 0x1080000},
            ShaderRegister{0x292, 0x22}, ShaderRegister{0x2f9, 0x2d}, ShaderRegister{0x1e0, 0},
            ShaderRegister{0x2fa, 0x3f800000}, ShaderRegister{0x2fb, 0x3f800000}, ShaderRegister{0x2fc, 0x3f800000}, ShaderRegister{0x2fd, 0x3f800000}
        };
        for (const auto value : state) sceAgcDcbSetCxRegisterDirect(&commands, value);
#if ANYUNREAL_TEST_DEPTH
        sceAgcDcbSetCxRegisterDirect(&commands, {0x200, 0x16});
        WriteDepthBias(commands, depthMode == 3 ? 8.0f : depthMode == 4 ? -8.0f : 0.0f, depthMode == 5 ? 1.0f : depthMode == 6 ? -1.0f : 0.0f);
        if (depthMode >= 3) sceAgcDcbSetCxRegisterDirect(&commands, {0x205, 0x240 | (7u << 11)});
#endif
        WriteViewport(commands, 0, {0, 0, 0, static_cast<float>(width), static_cast<float>(height), 1});
        WriteViewportScissor(commands, 0, {0, 0, width, height});
        WriteScissor(commands, {width / 8, height / 8, width - width / 8, height - height / 8});
        Shaders::UserDataInputs vertexInputs;
        descriptors.Apply(vertexInputs);
        vertexInputs.BaseVertex = indirect ? 19u : indexStride ? baseVertex - 1 : baseVertex;
        vertexInputs.StartInstance = firstInstance;
        WriteShaderRegisters(commands, vertex, vertexLayout, vertexInputs);
        WriteShaderRegisters(commands, pixel, pixelLayout, {});
#if ANYUNREAL_TEST_OCCLUSION
        OcclusionSnapshot occlusion(aperture);
        std::uint64_t sampleCount = 123;
        if (occlusion.TryGetResult(sampleCount, false) || sampleCount != 123) throw std::runtime_error("Unrecorded query returned a result");
        occlusion.Begin(commands);
#endif
        sceAgcDcbSetNumInstances(&commands, instanceCount);
        const auto draw = [&]
        {
            if (indirect) WriteIndirectDraw(commands, vertexLayout, argumentStorage, 4, indexStride ? &indexStorage : nullptr, indexStride);
            else if (indexStride)
            {
                sceAgcDcbSetIndexSize(&commands, indexStride == 2 ? 0 : 1, 0);
                sceAgcDcbDrawIndex(&commands, 3, static_cast<std::byte*>(indexStorage.Data()) + indexStride, 0);
            }
            else sceAgcDcbDrawIndexAuto(&commands, 3, 0);
        };
        draw();
#if ANYUNREAL_TEST_DEPTH
        if (depthMode >= 2)
        {
#if ANYUNREAL_TEST_DEPTH_BARRIER
            const auto& depthStorage = depthTexture.GetStorage();
            const std::array depthRanges{GpuMemoryRange{depthStorage.Data(), depthStorage.Size()}, GpuMemoryRange{output.Data(), output.Size()}};
            depthBarrier.Release(commands);
            depthBarrier.Acquire(commands, depthRanges);
#endif
            sceAgcDcbSetCxRegisterDirect(&commands, {0x205, 0x240});
            WriteColorTarget(commands, 0, secondTarget);
            draw();
        }
#endif
#if ANYUNREAL_TEST_OCCLUSION
        occlusion.End(commands);
        if (occlusion.TryGetResult(sampleCount, false) || sampleCount != 123) throw std::runtime_error("Unsubmitted query returned a result");
#endif
        SubmitAndWait(commands, fence, 1, 10000000);
#if ANYUNREAL_TEST_OCCLUSION
        const auto expectedSamples = static_cast<std::uint64_t>(width - 2 * (width / 8)) * (height - 2 * (height / 8)) * instanceCount;
#endif
        std::array<std::uint8_t, 16> color{};
        switch (format)
        {
        case TextureFormat::R8Unorm: color[0] = 128; break;
        case TextureFormat::Rg8Unorm: color[0] = 128; color[1] = 64; break;
        case TextureFormat::Rgba8Unorm: color[0] = 128; color[1] = 64; color[2] = 191; color[3] = 255; break;
        case TextureFormat::Rgba8Srgb: color[0] = 188; color[1] = 137; color[2] = 225; color[3] = 255; break;
        case TextureFormat::Bgra8Unorm: color[0] = 191; color[1] = 64; color[2] = 128; color[3] = 255; break;
        case TextureFormat::Bgra8Srgb: color[0] = 225; color[1] = 137; color[2] = 188; color[3] = 255; break;
        case TextureFormat::R32Float:
        case TextureFormat::Rgba32Float: break;
        case TextureFormat::R16Float: color = {0, 0x38}; break;
        case TextureFormat::Rg16Float: color = {0, 0x38, 0, 0x34}; break;
        case TextureFormat::Rgba16Float: color = {0, 0x38, 0, 0x34, 0, 0x3a, 0, 0x3c}; break;
        }
        const auto* pixels = static_cast<const std::uint8_t*>(output.Data());
#if ANYUNREAL_TEST_DEPTH
        if (depthMode >= 2) pixels = static_cast<const std::uint8_t*>(secondTarget.GetStorage().Data());
#endif
        const auto pixelBytes = targetTexture.GetRowBytes() / width;
        for (unsigned y = 0; y < height; ++y)
        {
            for (unsigned x = 0; x < width; ++x)
            {
                bool inside = x >= width / 8 && x < width - width / 8 && y >= height / 8 && y < height - height / 8;
#if ANYUNREAL_TEST_DEPTH
                inside = inside && (depthMode == 1 || depthMode == 3 || depthMode == 5);
#endif
                if (format == TextureFormat::R32Float || format == TextureFormat::Rgba32Float)
                {
                    const std::array<float, 4> values{0.5f, 0.25f, 0.75f, 1};
                    const auto* floats = reinterpret_cast<const float*>(pixels + y * targetTexture.GetRowPitch() + x * pixelBytes);
                    for (std::size_t component = 0; component < pixelBytes / sizeof(float); ++component)
                    {
                        const auto expected = inside ? values[component] : 0;
                        if (!std::isfinite(floats[component]) || std::abs(floats[component] - expected) > 0.000001f) throw std::runtime_error("Floating color target pixel mismatch for format " + std::to_string(static_cast<int>(format)) + " at " + std::to_string(x) + "," + std::to_string(y) + " component " + std::to_string(component) + ": " + std::to_string(floats[component]) + " expected " + std::to_string(expected) + " " + caseName);
                    }
                    continue;
                }
                for (std::size_t component = 0; component < pixelBytes; ++component)
                {
                    const auto expected = inside ? color[component] : 0;
                    const auto actual = pixels[y * targetTexture.GetRowPitch() + x * pixelBytes + component];
                    if (std::abs(static_cast<int>(actual) - expected) > (inside && format != TextureFormat::Rgba16Float && format != TextureFormat::R16Float && format != TextureFormat::Rg16Float ? 1 : 0)) throw std::runtime_error("Graphics pixel mismatch for format " + std::to_string(static_cast<int>(format)) + " at " + std::to_string(x) + "," + std::to_string(y) + ": " + std::to_string(actual) + " " + caseName);
                }
            }
        }
#if ANYUNREAL_TEST_OCCLUSION
        if (!occlusion.TryGetResult(sampleCount, true) || sampleCount != expectedSamples) throw std::runtime_error("Occlusion sample count mismatch: " + std::to_string(sampleCount) + " expected " + std::to_string(expectedSamples) + " format " + std::to_string(static_cast<int>(format)) + " indirect " + std::to_string(indirect));
#endif
    }
    std::cout << "Vertex buffer draw and scissor pixels verified\n";
    return 0;
}
