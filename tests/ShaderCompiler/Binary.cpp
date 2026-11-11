#include "AnyUnreal/Shader/ShaderBinary.h"
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>

namespace
{
    void require(bool condition)
    {
        if (!condition) throw std::runtime_error("Shader binary check failed");
    }

    template<class TCallable>
    void reject(TCallable callable)
    {
        try
        {
            callable();
        }
        catch (const std::invalid_argument&)
        {
            return;
        }
        throw std::runtime_error("Invalid shader binary was accepted");
    }
}

int main()
{
    using namespace AnyUnreal::Shader;
    const std::array<std::uint8_t, 5> payload{1, 2, 3, 4, 5};
    ShaderBinaryView shader{ShaderStage::Pixel, AgcTarget::Ps5, payload, payload, payload, payload, {
        {ResourceKind::UniformBuffer, 0, 1, 3, 1, 0, 16, 65536, "uniform-buffer"},
        {ResourceKind::ShaderResource, 2, 0, 2, 2, 0, 32, 0, "sampled-image"},
        {ResourceKind::Sampler, 7, 0, 7, 1, 64, 16, 0, "sampler"}
    }};
    const auto bytes = EncodeShaderBinary(shader);
    require(bytes[0] == 'A' && bytes[1] == 'U' && bytes[2] == 'G' && bytes[3] == 'S');
    const auto decoded = DecodeShaderBinary(bytes);
    require(decoded.Stage == shader.Stage && decoded.Target == shader.Target);
    require(decoded.Bindings.size() == 3 && decoded.Bindings[0].BlockSize == 65536);
    require(decoded.Bindings[1].Type == "sampled-image" && decoded.Bindings[1].Count == 2);
    require(decoded.Header.data() >= bytes.data() && decoded.Header.data() < bytes.data() + bytes.size());
    for (auto section : {decoded.Header, decoded.Code, decoded.Metadata, decoded.ResourceTable}) require(std::ranges::equal(section, payload));
    require(EncodeShaderBinary(decoded) == bytes);
    auto unreal = bytes;
    unreal.insert(unreal.end(), {4, 0, 0, 0});
    require(EncodeShaderBinary(DecodeUnrealShaderCode(unreal)) == bytes);
    unreal = bytes;
    unreal.insert(unreal.end(), {0x75, 2, 0, 0, 0, 0xab, 0xcd, 11, 0, 0, 0});
    require(EncodeShaderBinary(DecodeUnrealShaderCode(unreal)) == bytes);
    for (std::size_t size = 0; size < unreal.size(); ++size) reject([&] { DecodeUnrealShaderCode(std::span(unreal).first(size)); });
    auto invalidOptional = unreal;
    invalidOptional[bytes.size() + 1] = 255;
    reject([&] { DecodeUnrealShaderCode(invalidOptional); });
    invalidOptional = bytes;
    invalidOptional.insert(invalidOptional.end(), {0x75, 2, 0, 0, 0, 0xab, 0xcd, 0x75, 2, 0, 0, 0, 0xab, 0xcd, 18, 0, 0, 0});
    reject([&] { DecodeUnrealShaderCode(invalidOptional); });
    invalidOptional = unreal;
    std::fill(invalidOptional.end() - 4, invalidOptional.end(), 0xff);
    reject([&] { DecodeUnrealShaderCode(invalidOptional); });
    std::fill(invalidOptional.end() - 4, invalidOptional.end(), 0);
    reject([&] { DecodeUnrealShaderCode(invalidOptional); });
    for (std::size_t size = 0; size < bytes.size(); ++size) reject([&] { DecodeShaderBinary(std::span(bytes).first(size)); });
    for (std::size_t offset : {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 52, 60, 68})
    {
        auto corrupt = bytes;
        std::fill_n(corrupt.begin() + offset, 4, 255);
        reject([&] { DecodeShaderBinary(corrupt); });
    }
    auto trailing = bytes;
    trailing.push_back(0);
    reject([&] { DecodeShaderBinary(trailing); });
    auto invalid = shader;
    invalid.Bindings.push_back(invalid.Bindings[0]);
    reject([&] { EncodeShaderBinary(invalid); });
    invalid = shader;
    invalid.Bindings[2].Offset = 16;
    reject([&] { EncodeShaderBinary(invalid); });
    invalid = shader;
    invalid.Bindings[1].Stride = 16;
    reject([&] { EncodeShaderBinary(invalid); });
    invalid = shader;
    invalid.Header = {};
    reject([&] { EncodeShaderBinary(invalid); });
    std::cout << "Shader binary round trip, bounds and binding validation passed\n";
}
