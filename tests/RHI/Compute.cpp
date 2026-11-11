#include "AnyUnreal/RHI/Memory/BufferAllocation.h"
#include "AnyUnreal/RHI/Descriptor/Tables.h"
#include "AnyUnreal/RHI/Descriptor/BufferDescriptor.h"
#include "AnyUnreal/RHI/Descriptor/MergedTables.h"
#include "AnyUnreal/RHI/Descriptor/SamplerDescriptor.h"
#include "AnyUnreal/Shader/ConstantData.h"
#include <algorithm>
#include <string_view>
#include <string>
#include "AnyUnreal/RHI/Command/Submission.h"
#include "AnyUnreal/RHI/Command/ComputeDispatch.h"
#include "AnyUnreal/RHI/Command/Viewport.h"
#include "AnyUnreal/RHI/Command/DepthBounds.h"
#include "AnyUnreal/RHI/Command/MemoryAcquire.h"
#include "AnyUnreal/RHI/Command/BufferFill.h"
#include "AnyUnreal/RHI/Command/PatternFill.h"
#include "AnyUnreal/RHI/Buffer/BufferClearValue.h"
#include "AnyUnreal/RHI/Command/MemoryBarrier.h"
#include "AnyUnreal/RHI/Command/CommandStorage.h"
#include "AnyUnreal/RHI/Command/Completion.h"
#include "AnyUnreal/RHI/Command/BufferCopy.h"
#include "AnyUnreal/RHI/Memory/Readback.h"
#include "AnyUnreal/RHI/Memory/Upload.h"
#include "AnyUnreal/RHI/Texture/LinearTexture2D.h"
#include "AnyUnreal/RHI/Texture/LinearTexture3D.h"
#include "AnyUnreal/RHI/Memory/HalfFloat.h"
#include "AnyUnreal/RHI/Command/DepthTarget.h"
#include "prx/libSceAgc/DcbState/include/Marker.hpp"
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
        const std::array secondBindings{Shaders::RuntimeBinding{Kind::UniformBuffer, 7, 2, 10, 1, 80, 16, 64, "uniform-buffer"}};
        RHI::DescriptorTables second(secondBindings, addressHigh);
        const std::array<const RHI::DescriptorTables*, 2> sources{&tables, &second};
        reject([&] { RHI::MergedDescriptorTables incomplete(sources, addressHigh); });
        second.SetBuffer(Kind::UniformBuffer, 7, buffer, 256);
        RHI::MergedDescriptorTables merged(sources, addressHigh);
        Shaders::UserDataInputs mergedInputs;
        merged.Apply(mergedInputs);
        const auto* mergedWords = reinterpret_cast<const std::uint32_t*>(mergedInputs.DescriptorSets[2]);
        if (mergedWords[10] != 64 || mergedWords[14] != 128 || mergedWords[22] != 256) throw std::runtime_error("Merged descriptors lost stage resources");
        if (mergedInputs.DescriptorSets[2] == inputs.DescriptorSets[2] || merged.GetMemoryRanges().size() != 2) throw std::runtime_error("Merged descriptor storage aliases a source");
        for (unsigned index = 16; index < 20; ++index)
        {
            if (mergedWords[index] != 0) throw std::runtime_error("Merged descriptor padding is not zero");
        }
        second.SetBuffer(Kind::UniformBuffer, 7, buffer, 512);
        if (mergedWords[22] != 256) throw std::runtime_error("Merged descriptors changed after source mutation");
        reject([&] { RHI::MergedDescriptorTables duplicate(std::array<const RHI::DescriptorTables*, 2>{&tables, &tables}, addressHigh); });
        reject([&] { RHI::MergedDescriptorTables nullSource(std::array<const RHI::DescriptorTables*, 2>{&tables, nullptr}, addressHigh); });
        for (const auto offset : {48u, 128u})
        {
            auto overlappingBinding = secondBindings;
            overlappingBinding[0].Offset = offset;
            if (offset == 128) overlappingBinding[0].Binding = bindings[0].Binding;
            RHI::DescriptorTables overlapping(overlappingBinding, addressHigh);
            overlapping.SetBuffer(Kind::UniformBuffer, 7, buffer, 64);
            reject([&] { RHI::MergedDescriptorTables collision(std::array<const RHI::DescriptorTables*, 2>{&tables, &overlapping}, addressHigh); });
        }
        reject([&] { RHI::CreateTypedBufferDescriptor(buffer, 3, RHI::BufferFormat::Uint1); });
        reject([&] { RHI::CreateTypedBufferDescriptor(static_cast<const std::byte*>(buffer) + 1, 64, RHI::BufferFormat::Uint1); });
        reject([&] { RHI::CreateTypedBufferDescriptor(buffer, 64, RHI::BufferFormat::Untyped); });
        for (unsigned index = 0; index < 8; ++index)
        {
            if (array[index] != 0) throw std::runtime_error("Descriptor write overwrote layout padding");
        }
    }

}

int main()
{
    {
        using namespace AnyUnreal::RHI;
        const std::array<std::uint32_t, 4> values{0x12345678u, 0x9abcdef0u, 0x13579bdfu, 0x2468ace0u};
        const auto check = [](BufferFormat format, const auto& input, std::array<std::uint32_t, 4> expected)
        {
            if (EncodeBufferClear(format, input) != expected) throw std::runtime_error("Buffer clear component encoding mismatch");
        };
        check(BufferFormat::Byte4Uint, values, {0xe0dff078u, 0, 0, 0});
        check(BufferFormat::Bgra8Unorm, values, {0xe078f0dfu, 0, 0, 0});
        check(BufferFormat::Short2Uint, values, {0xdef05678u, 0, 0, 0});
        check(BufferFormat::Short4Sint, values, {0xdef05678u, 0xace09bdfu, 0, 0});
        check(BufferFormat::Float3, values, {values[0], values[1], values[2], 0});
        check(BufferFormat::Float4, values, values);
        check(BufferFormat::Untyped, values, {values[0], 0, 0, 0});
        const std::array<float, 4> floats{1.0f, -1.0f, 0.5f, 0.0f};
        check(BufferFormat::Half4, floats, {0xbc003c00u, 0x00003800u, 0, 0});
        check(BufferFormat::Byte4Unorm, floats, {0x008000ffu, 0, 0, 0});
        check(BufferFormat::Byte4Snorm, floats, {0x0040817fu, 0, 0, 0});
        check(BufferFormat::Short4Snorm, floats, {0x80017fffu, 0x00004000u, 0, 0});
        check(BufferFormat::Float2, floats, {0x3f800000u, 0xbf800000u, 0, 0});
        check(BufferFormat::Untyped, floats, {0x3f800000u, 0, 0, 0});
        bool rejected = false;
        try { EncodeBufferClear(BufferFormat::Uint1, floats); }
        catch (const std::invalid_argument&) { rejected = true; }
        if (!rejected) throw std::runtime_error("Float clear accepted an integer buffer format");
    }

    if (getargc_nid_postfix() != 5) throw std::invalid_argument("Expected compute header, code, metadata and operation");
    const auto* arguments = getargv_nid_postfix();
    const std::string_view operation(arguments[4]);
    const bool layerView = operation == "layer-view";
    const bool layerRead = operation == "layer-texture" || layerView;
    const bool cubeArray = operation == "cube-array" || operation == "cube-array-view";
    const bool cubeView = operation == "cube-view" || operation == "cube-array-view";
    const bool cubeRead = operation == "cube-texture" || cubeView || cubeArray;
    const bool withSampler = cubeRead || operation == "sample-wrap" || operation == "sample-clamp" || operation == "sample-mirror" || operation == "sample-border" || operation == "sample-linear";
    if (!layerRead && !withSampler && operation != "write" && operation != "read-write" && operation != "typed-buffer" && operation != "constants" && operation != "texture" && operation != "mip-texture" && operation != "mip-view" && operation != "texture-view" && operation != "texture-write" && operation != "depth-texture" && operation != "half-texture" && operation != "volume-texture" && operation != "volume-write" && operation != "volume-mip-texture" && operation != "volume-mip-view" && operation != "array-texture" && operation != "array-view") throw std::invalid_argument("Unknown compute operation");
    const bool volumeMipView = operation == "volume-mip-view";
    const bool volumeMipRead = operation == "volume-mip-texture" || volumeMipView;
    const bool volumeRead = operation == "volume-texture" || operation == "volume-write" || volumeMipRead;
    const bool arrayView = operation == "array-view";
    const bool arrayRead = operation == "array-texture" || arrayView;
    const bool mipView = operation == "mip-view" || arrayView || cubeView || layerView;
    const bool mipRead = operation == "mip-texture" || mipView || arrayRead || cubeRead || layerRead;
    const bool halfRead = operation == "half-texture";
    const bool depthRead = operation == "depth-texture";
    const bool textureWrite = operation == "texture-write" || operation == "volume-write";
    const bool textureView = operation == "texture-view";
    const bool withTexture = operation == "texture" || mipRead || textureView || withSampler || (textureWrite && !volumeRead) || halfRead;
    const bool typedBuffer = operation == "typed-buffer";
    const bool readWrite = operation == "read-write" || typedBuffer;
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
    std::vector<Shaders::RuntimeBinding> bindings;
    std::unique_ptr<RHI::LinearTexture2D> texture;
    if (withTexture)
    {
        texture = std::make_unique<RHI::LinearTexture2D>(mipView ? 16u : 8u, mipView ? 16u : 8u, textureView ? RHI::TextureFormat::Rgba8Srgb : halfRead ? RHI::TextureFormat::Rgba16Float : RHI::TextureFormat::R32Float, layout.GetAddressHigh(), AnyUnreal::Kernel::DirectAllocation::PageSize, mipView ? 5u : mipRead ? 4u : 1u, RHI::AllocationUsage::Texture, cubeArray ? 18u : cubeRead ? 12u : (arrayRead || layerRead) ? 5u : 1u);
        std::array<float, 64> texels;
        for (unsigned index = 0; index < texels.size(); ++index) texels[index] = static_cast<float>(textureWrite ? index * 7 + 1000 : index * 3 + 1);
        if (textureView)
        {
            std::array<std::uint32_t, 64> colors;
            for (unsigned index = 0; index < colors.size(); ++index) colors[index] = 0xff000000u | (index * 3 + 1);
            texture->Upload(std::as_bytes(std::span(colors)), 8 * sizeof(std::uint32_t));
        }
        else if (halfRead)
        {
            std::array<std::uint16_t, 256> colors;
            for (std::size_t index = 0; index < texels.size(); ++index)
            {
                colors[index * 4] = RHI::EncodeHalfFloat(texels[index]);
                colors[index * 4 + 1] = 0xbc00;
                colors[index * 4 + 2] = 0x3800;
                colors[index * 4 + 3] = 0x3c00;
            }
            texture->Upload(std::as_bytes(std::span(colors)), 8 * 4 * sizeof(std::uint16_t));
        }
        else if (!mipRead) texture->Upload(std::as_bytes(std::span(texels)), 8 * sizeof(float));
        if (mipRead)
        {
            for (std::uint32_t layer = 0; layer < texture->GetArrayLayers(); ++layer)
            for (std::uint32_t level = 0; level < texture->GetMipCount(); ++level)
            {
                const auto& mip = texture->GetMipLayout(level);
                std::vector<float> pixels(mip.Width * mip.Height);
                for (std::size_t pixel = 0; pixel < pixels.size(); ++pixel) pixels[pixel] = static_cast<float>(layer * 1000 + level * 100 + (cubeRead ? 0 : pixel * 3) + 1);
                texture->Upload(std::as_bytes(std::span(pixels)), mip.RowBytes, level, layer);
                std::vector<float> copied(pixels.size());
                texture->CopyRowsTo(std::as_writable_bytes(std::span(copied)), mip.RowBytes, level, layer);
                if (copied != pixels) throw std::runtime_error("Mip upload or readback changed texels");
            }
        }
        bool invalidPitchRejected = false;
        try { texture->Upload(std::as_bytes(std::span(texels)), 16 * sizeof(float)); }
        catch (const std::invalid_argument&) { invalidPitchRejected = true; }
        if (!invalidPitchRejected || texture->GetRowPitch() != 256) throw std::runtime_error("Texture upload accepted incomplete rows or used an invalid row pitch");
        bindings.push_back({textureWrite ? Shaders::ResourceKind::UnorderedAccess : Shaders::ResourceKind::ShaderResource, 0, 0, 0, 1, 0, 32, 0, textureWrite ? "storage-image" : "sampled-image"});
    }
    std::unique_ptr<RHI::LinearTexture3D> volume;
    if (volumeRead)
    {
        if (volumeMipRead)
        {
            const auto extent = volumeMipView ? 16u : 8u;
            volume = std::make_unique<RHI::LinearTexture3D>(extent, extent, extent, RHI::TextureFormat::R32Float, layout.GetAddressHigh(), volumeMipView ? 5u : 4u);
            for (std::uint32_t level = 0; level < volume->GetMipCount(); ++level)
            {
                const auto& mip = volume->GetMipLayout(level);
                std::vector<float> values(mip.Width * mip.Height * mip.Depth);
                for (std::size_t index = 0; index < values.size(); ++index) values[index] = static_cast<float>(level * 1000 + index * 3 + 1);
                volume->Upload(std::as_bytes(std::span(values)), mip.RowBytes, mip.RowBytes * mip.Height, level);
                std::vector<float> copied(values.size());
                volume->CopyTo(std::as_writable_bytes(std::span(copied)), mip.RowBytes, mip.RowBytes * mip.Height, level);
                if (copied != values) throw std::runtime_error("Volume mip upload or readback changed texels");
            }
        }
        else
        {
            volume = std::make_unique<RHI::LinearTexture3D>(5, 3, 5, RHI::TextureFormat::R32Float, layout.GetAddressHigh());
            std::array<float, 75> values{};
            for (std::size_t index = 0; index < values.size(); ++index) values[index] = static_cast<float>(textureWrite ? index * 7 + 1000 : index * 3 + 1);
            const auto source = std::as_bytes(std::span(values));
            volume->Upload(source, 5 * sizeof(float), 15 * sizeof(float));
            bool rejected = false;
            try { volume->Upload(source.first(source.size() - 1), 5 * sizeof(float), 15 * sizeof(float)); }
            catch (const std::invalid_argument&) { rejected = true; }
            if (!rejected) throw std::runtime_error("Incomplete volume upload was accepted");
            std::array<float, 75> copied{};
            volume->CopyTo(std::as_writable_bytes(std::span(copied)), 5 * sizeof(float), 15 * sizeof(float));
            if (copied != values || volume->GetRowPitch() != 256 || volume->GetSlicePitch() != 768) throw std::runtime_error("Volume layout did not preserve its texels");
        }
        bindings.push_back({textureWrite ? Shaders::ResourceKind::UnorderedAccess : Shaders::ResourceKind::ShaderResource, 0, 0, 0, 1, 0, 32, 0, textureWrite ? "storage-image" : "sampled-image"});
    }
    std::unique_ptr<RHI::DepthTexture2D> depthTexture;
    if (depthRead)
    {
        depthTexture = std::make_unique<RHI::DepthTexture2D>(259, 137, layout.GetAddressHigh());
        bindings.push_back({Shaders::ResourceKind::ShaderResource, 0, 0, 0, 1, 0, 32, 0, "sampled-image"});
    }
    if (readWrite) bindings.push_back({Shaders::ResourceKind::ShaderResource, 0, 0, 0, 1, 0, 16, 0, typedBuffer ? "uniform-texel-buffer" : "storage-buffer"});
    if (withSampler) bindings.push_back({Shaders::ResourceKind::Sampler, 0, 0, 1, 1, 32, 16, 0, "sampler"});
    bindings.push_back({Shaders::ResourceKind::UnorderedAccess, textureWrite ? 1u : 0u, 0, withSampler ? 2u : readWrite || withTexture || depthRead || volumeRead ? 1u : 0u, 1, withSampler ? 48u : withTexture || depthRead || volumeRead ? 32u : readWrite ? 16u : 0u, 16, 0, typedBuffer ? "storage-texel-buffer" : "storage-buffer"});
    if (withConstants) bindings.push_back({Shaders::ResourceKind::UniformBuffer, 0, 1, 0, 1, 0, 16, 16, "uniform-buffer"});
    Shaders::ConstantData constantValues(bindings);
    RHI::DescriptorTables descriptors(bindings, layout.GetAddressHigh());
    Shaders::UserDataInputs inputs;
    bool rejected = false;
    try { descriptors.Apply(inputs); }
    catch (const std::runtime_error&) { rejected = true; }
    if (!rejected) throw std::runtime_error("Uninitialized descriptor table was accepted");
    const auto bindBuffer = [&](RHI::DescriptorTables& tables, Shaders::ResourceKind kind, std::uint32_t index, const void* data)
    {
        if (typedBuffer)
        {
            const auto descriptor = RHI::CreateTypedBufferDescriptor(data, 64 * sizeof(std::uint32_t), RHI::BufferFormat::Uint1);
            tables.SetDescriptor(kind, index, std::as_bytes(std::span(descriptor)));
        }
        else tables.SetBuffer(kind, index, data, 64 * sizeof(std::uint32_t));
    };
    bindBuffer(descriptors, Shaders::ResourceKind::UnorderedAccess, textureWrite ? 1u : 0u, output);
    if (readWrite) bindBuffer(descriptors, Shaders::ResourceKind::ShaderResource, 0, input);
    if (volume)
    {
        const auto descriptor = volume->CreateViewDescriptor(false, volumeMipView ? 1u : 0u);
        descriptors.SetDescriptor(textureWrite ? Shaders::ResourceKind::UnorderedAccess : Shaders::ResourceKind::ShaderResource, 0, std::as_bytes(std::span(descriptor)));
    }
    if (depthTexture)
    {
        const auto descriptor = depthTexture->GetDescriptor();
        descriptors.SetDescriptor(Shaders::ResourceKind::ShaderResource, 0, std::as_bytes(std::span(descriptor)));
    }
    if (texture)
    {
        const auto original = texture->GetDescriptor();
        const auto view = cubeRead ? texture->CreateCubeViewDescriptor(false, cubeView ? 1u : 0u, cubeArray ? 2u : 1u, mipView ? 1u : 0u) : arrayRead ? texture->CreateArrayViewDescriptor(false, arrayView ? 2u : 0u, 3, mipView ? 1u : 0u) : texture->CreateViewDescriptor(textureView, mipView ? 1u : 0u, 0, layerRead ? 3u : 0u);
        if (original != texture->GetDescriptor() || (textureView && view == original)) throw std::runtime_error("Texture view did not preserve the resource descriptor or disable sRGB");
        descriptors.SetDescriptor(textureWrite ? Shaders::ResourceKind::UnorderedAccess : Shaders::ResourceKind::ShaderResource, 0, std::as_bytes(std::span(view)));
    }
    if (withSampler)
    {
        RHI::SamplerDescription description;
        if (operation == "sample-clamp") description.AddressU = RHI::SamplerAddress::Clamp;
        if (operation == "sample-mirror") description.AddressU = RHI::SamplerAddress::Mirror;
        if (operation == "sample-border") description.AddressU = RHI::SamplerAddress::Border;
        if (operation == "sample-linear") description.Filter = RHI::SamplerFilter::Linear;
        const auto sampler = RHI::CreateSamplerDescriptor(description);
        descriptors.SetDescriptor(Shaders::ResourceKind::Sampler, 0, std::as_bytes(std::span(sampler)));
    }
    if (withConstants) descriptors.SetBuffer(Shaders::ResourceKind::UniformBuffer, 0, memory.GetRange(1024, 16).data(), 16);
    descriptors.Apply(inputs);
    std::unique_ptr<RHI::MergedDescriptorTables> mergedDescriptors;
    if (readWrite)
    {
        RHI::DescriptorTables inputDescriptors(std::span(bindings).first(1), layout.GetAddressHigh());
        RHI::DescriptorTables outputDescriptors(std::span(bindings).last(1), layout.GetAddressHigh());
        bindBuffer(inputDescriptors, Shaders::ResourceKind::ShaderResource, 0, input);
        bindBuffer(outputDescriptors, Shaders::ResourceKind::UnorderedAccess, 0, output);
        mergedDescriptors = std::make_unique<RHI::MergedDescriptorTables>(std::array<const RHI::DescriptorTables*, 2>{&inputDescriptors, &outputDescriptors}, layout.GetAddressHigh());
        mergedDescriptors->Apply(inputs);
    }
    for (std::uint64_t iteration = 1; iteration <= 3; ++iteration)
    {
        RHI::MemoryBarrier barrier(layout.GetAddressHigh());
        RHI::Completion completion(layout.GetAddressHigh());
        RHI::Readback readback(249, layout.GetAddressHigh());
        RHI::MemoryBarrier readbackBarrier(layout.GetAddressHigh());
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
        auto* fillOutput = reinterpret_cast<std::uint32_t*>(memory.GetRange(3072, 64).data());
        std::fill_n(fillOutput, 16, 0x12345678u);
        auto patternOutput = memory.GetRange(3200, 256);
        std::fill(patternOutput.begin(), patternOutput.end(), std::byte{0x39});
        const std::array<std::uint32_t, 4> fillPattern{0x76543210u, 0xfedcba98u, static_cast<std::uint32_t>(iteration), 0x80000000u};
        auto copyOutput = memory.GetRange(4096, 272);
        std::fill(copyOutput.begin(), copyOutput.end(), std::byte{0x5a});
        std::array<std::byte, 31> uploadSource;
        for (std::size_t index = 0; index < uploadSource.size(); ++index) uploadSource[index] = static_cast<std::byte>(index + iteration);
        const auto uploadExpected = uploadSource;
        RHI::Upload upload(uploadSource, layout.GetAddressHigh());
        uploadSource.fill(std::byte{0});
        auto uploadOutput = memory.GetRange(4608, 40);
        std::fill(uploadOutput.begin(), uploadOutput.end(), std::byte{0x7b});
        RHI::CommandStorage commandStorage(64, layout.GetAddressHigh());
        auto& commands = commandStorage.Get();
        sceAgcDcbContextStateOp(&commands, 0);
        sceAgcDcbPushMarker(&commands, "ComputeCheck", 0xff4080c0);
        std::vector<RHI::GpuMemoryRange> ranges{
            {memory.Data(), memory.Size()},
            {secondMemory.Data(), secondMemory.Size()},
            {shader.Get().code, shader.Get().shader_size}
        };
        const auto originalPosition = commands.cursor_up;
        for (const auto bounds : {std::array{-0.1f, 1.0f}, std::array{0.0f, 1.1f}, std::array{std::numeric_limits<float>::quiet_NaN(), 1.0f}, std::array{0.0f, std::numeric_limits<float>::infinity()}})
        {
            bool rejectedBounds = false;
            try { RHI::WriteDepthBounds(commands, bounds[0], bounds[1]); }
            catch (const std::invalid_argument&) { rejectedBounds = true; }
            if (!rejectedBounds || commands.cursor_up != originalPosition) throw std::runtime_error("Invalid depth bounds changed the command buffer");
        }
        if (texture) ranges.push_back({texture->GetStorage().Data(), texture->GetStorage().Size()});
        if (volume) ranges.push_back({volume->GetStorage().Data(), volume->GetStorage().Size()});
        if (depthTexture) ranges.push_back({depthTexture->GetStorage().Data(), depthTexture->GetStorage().Size()});
        for (const RHI::Viewport viewport : {RHI::Viewport{0, 0, 0, -1, 1, 1}, RHI::Viewport{0, 0, -1, 1, 1, 1}, RHI::Viewport{0, 0, 0, std::numeric_limits<float>::infinity(), 1, 1}})
        {
            bool rejected = false;
            try { RHI::WriteViewport(commands, 0, viewport); }
            catch (const std::logic_error&) { rejected = true; }
            if (!rejected || commands.cursor_up != originalPosition) throw std::runtime_error("Invalid viewport changed the command buffer");
        }
        bool scissorRejected = false;
        try { RHI::WriteScissor(commands, {0, 0, 32768, 1}); }
        catch (const std::out_of_range&) { scissorRejected = true; }
        if (!scissorRejected || commands.cursor_up != originalPosition) throw std::runtime_error("Invalid scissor changed the command buffer");
        bool overlappingCopyRejected = false;
        try { RHI::WriteBufferCopy(commands, copyOutput.data() + 1, copyOutput.data(), 8); }
        catch (const std::invalid_argument&) { overlappingCopyRejected = true; }
        if (!overlappingCopyRejected || commands.cursor_up != originalPosition) throw std::runtime_error("Overlapping GPU copy changed the command buffer");
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
        for (const auto invalid : {0u, 1u, 7u, 13u})
        {
            bool rejected = false;
            try { RHI::WritePatternFill(commands, fillOutput, invalid, std::span(fillPattern).first(3)); }
            catch (const std::invalid_argument&) { rejected = true; }
            if (!rejected || commands.cursor_up != originalPosition) throw std::runtime_error("Invalid pattern fill changed the command buffer");
        }
        if (depthTexture) RHI::WriteDepthClear(commands, *depthTexture, iteration == 1 ? 1.0f : 0.0f);
        RHI::WriteViewport(commands, 0, {0, 0, 0, 1920, 1080, 1});
        RHI::WriteViewport(commands, 15, {16, 24, 1, 256, 128, 0});
        RHI::WriteViewportScissor(commands, 0, {0, 0, 1920, 1080});
        RHI::WriteViewportScissor(commands, 15, {16, 24, 256, 128});
        RHI::WriteScissor(commands, {0, 0, 32767, 32767});
        const auto tableRanges = mergedDescriptors ? mergedDescriptors->GetMemoryRanges() : descriptors.GetMemoryRanges();
        ranges.insert(ranges.end(), tableRanges.begin(), tableRanges.end());
        RHI::WriteMemoryAcquire(commands, ranges);
        upload.Record(commands, uploadOutput.subspan(3, uploadExpected.size()));
        RHI::WriteBufferCopy(commands, copyOutput.data() + 7, input, sizeof(values));
        RHI::WriteBufferFill(commands, fillOutput + 4, 32, static_cast<std::uint32_t>(iteration));
        for (std::size_t count = 1; count <= fillPattern.size(); ++count)
        {
            RHI::WritePatternFill(commands, patternOutput.data() + (count - 1) * 64 + 4, 48, std::span(fillPattern).first(count));
        }
        if (readWrite) RHI::WriteBufferFill(commands, input, 64 * sizeof(std::uint32_t), static_cast<std::uint32_t>(iteration));
        barrier.Release(commands);
        barrier.Acquire(commands, ranges);
        if (iteration == 1) RHI::WriteComputeDispatch(commands, shader, layout, inputs, 1, 1, 1);
        else RHI::WriteIndirectComputeDispatch(commands, shader, layout, inputs, memory.Data(), memory.Size(), 2048);
        readbackBarrier.Release(commands);
        readbackBarrier.Acquire(commands, ranges);
        const auto readbackSource = std::as_bytes(std::span(output, 64)).subspan(3, 249);
        readback.Record(commands, readbackSource);
        bool prematureReadRejected = false;
        try { readback.GetRange(0, 249); }
        catch (const std::runtime_error&) { prematureReadRejected = true; }
        if (!prematureReadRejected) throw std::runtime_error("Readback accepted a read before GPU submission");
        sceAgcDcbPopMarker(&commands);
        completion.Write(commands);
        if (completion.IsComplete()) throw std::runtime_error("Unsubmitted commands reported completion");
        RHI::WriteDepthBounds(commands, 0.25f, 0.75f);
        const auto* depthBounds = commands.cursor_up - 6;
        if (depthBounds[0] != 0xc0016900u || depthBounds[1] != 8 || depthBounds[2] != 0x3e800000u || depthBounds[3] != 0xc0016900u || depthBounds[4] != 9 || depthBounds[5] != 0x3f400000u) throw std::runtime_error("Depth bounds packet does not match RDNA2 registers");
        RHI::SubmitAndWait(commands, *fence, iteration, 10000000);
        completion.Wait(10000000);
        for (std::size_t index = 0; index < uploadOutput.size(); ++index)
        {
            const auto expectedByte = index >= 3 && index < 3 + uploadExpected.size() ? uploadExpected[index - 3] : std::byte{0x7b};
            if (uploadOutput[index] != expectedByte) throw std::runtime_error("GPU upload lost its source snapshot or changed bytes outside its destination");
        }
        const auto result = readback.GetRange(0, 249);
        if (!std::equal(result.begin(), result.end(), readbackSource.begin())) throw std::runtime_error("Readback differs from compute shader output");
        for (std::size_t index = 0; index < copyOutput.size(); ++index)
        {
            const auto expectedByte = index >= 7 && index < 7 + sizeof(values) ? std::as_bytes(std::span(values))[index - 7] : std::byte{0x5a};
            if (copyOutput[index] != expectedByte) throw std::runtime_error("GPU copy changed bytes outside its destination or copied incorrect data");
        }
        if (!completion.IsComplete()) throw std::runtime_error("Submitted commands did not report completion");
        for (std::size_t count = 1; count <= fillPattern.size(); ++count)
        for (std::size_t offset = 0; offset < 64; ++offset)
        {
            const auto expected = offset >= 4 && offset < 52 ? std::as_bytes(std::span(fillPattern).first(count))[(offset - 4) % (count * 4)] : std::byte{0x39};
            if (patternOutput[(count - 1) * 64 + offset] != expected) throw std::runtime_error("GPU pattern fill wrote incorrect elements or changed guard bytes");
        }
        for (unsigned index = 0; index < 16; ++index)
        {
            if (fillOutput[index] != (index >= 4 && index < 12 ? iteration : 0x12345678u)) throw std::runtime_error("GPU fill changed data outside its range or failed to write its value");
        }
        for (unsigned index = 0; index < 64; ++index)
        {
            if (readWrite && input[index] != iteration) throw std::runtime_error("GPU producer did not fill the compute input");
            auto expected = withConstants ? index * constants[0] + constants[1] : (readWrite ? iteration : index) * 3 + 1;
            if (volumeRead) expected = index * 3 + 1;
            if (volumeMipRead)
            {
                const auto level = index % 4;
                const auto width = 8u >> level;
                const auto voxel = index / 4;
                const auto z = (voxel * 3 + 1) % width;
                const auto pixel = (z * width + (voxel / width) % width) * width + voxel % width;
                expected = (level + (volumeMipView ? 1u : 0u)) * 1000 + pixel * 3 + 1;
            }
            if (mipRead)
            {
                const auto level = index % 4;
                const auto width = 8u >> level;
                expected = (level + (mipView ? 1u : 0u)) * 100 + ((index / 4) % (width * width)) * 3 + 1;
            }
            if (layerRead) expected += 3000;
            if (arrayRead) expected += ((index / 4) % 3 + (arrayView ? 2u : 0u)) * 1000;
            if (depthRead) expected = iteration == 1 ? 1 : 0;
            if (textureWrite) expected = ((index + 1) % 64) * 3 + 1;
            if (withSampler)
            {
                auto sourceIndex = index;
                if (operation == "sample-clamp") sourceIndex = (index / 8) * 8 + 7;
                if (operation == "sample-mirror") sourceIndex = (index / 8) * 8 + 7 - index % 8;
                expected = (sourceIndex * 3 + 1) * 4;
                if (operation == "sample-border") expected = 0;
                if (operation == "sample-linear") expected = (index * 3 + 1) * 3 + ((index / 8) * 8 + (index + 1) % 8) * 3 + 1;
            }
            if (cubeRead) expected = ((index % 6) + (cubeView ? 6u : 0u) + (cubeArray ? ((index / 24) % 2) * 6 : 0u)) * 1000 + ((index / 6) % 4 + (cubeView ? 1u : 0u)) * 100 + 1;
            if (output[index] != (iteration == 3 ? 0xffffffff : expected)) throw std::runtime_error("GPU compute output mismatch at " + std::to_string(index) + ": actual=" + std::to_string(output[index]) + ", expected=" + std::to_string(iteration == 3 ? 0xffffffff : expected) + ", iteration=" + std::to_string(iteration));
        }
    }
    std::cout << "GPU compute output verified for direct, indirect and zero-sized indirect dispatches\n";
}
