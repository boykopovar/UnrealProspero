#include "UnrealProspero/Shader/ShaderBinary.h"
#include "UnrealProspero/Shader/ConstantData.h"
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
    using namespace UnrealProspero::Shader;
    const std::array<std::uint8_t, 5> payload{1, 2, 3, 4, 5};
    ShaderBinaryView shader{ShaderStage::Pixel, AgcTarget::Ps5, payload, payload, payload, payload, {
        {ResourceKind::UniformBuffer, 0, 1, 3, 1, 0, 16, 65536, "uniform-buffer"},
        {ResourceKind::ShaderResource, 2, 0, 2, 2, 0, 32, 0, "sampled-image"},
        {ResourceKind::Sampler, 7, 0, 7, 1, 64, 16, 0, "sampler"}
    }};
    ConstantData constants(shader.Bindings);
    require(!constants.Contains(0));
    const std::array<std::byte, 8> source{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5}, std::byte{6}, std::byte{7}, std::byte{8}};
    constants.Write(0, 65528, source, 0, source.size());
    const auto snapshot = constants;
    constants.Write(0, 65530, source, 4, 4);
    require(snapshot.GetBlocks().at(0).Data[65530] == std::byte{3});
    require(constants.GetBlocks().at(0).Data[65530] == std::byte{5});
    require(constants.GetBlocks().at(0).Data[65528] == std::byte{1});
    require(constants.GetBlocks().at(0).Data[0] == std::byte{0});
    const auto beforeInvalidWrite = constants.GetBlocks().at(0).Data;
    const auto rejectRange = [](auto action)
    {
        try { action(); }
        catch (const std::out_of_range&) { return; }
        throw std::runtime_error("Invalid constant range was accepted");
    };
    rejectRange([&] { constants.Write(0, 65532, source, 0, 8); });
    rejectRange([&] { constants.Write(0, 0, source, 7, 2); });
    rejectRange([&] { constants.Write(0, 0, source, 0, 0); });
    reject([&] { constants.Write(1, 0, source, 0, 4); });
    require(constants.GetBlocks().at(0).Data == beforeInvalidWrite);
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
    for (std::size_t offset : {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 52, 56, 60, 64, 68, 72, 80, 84, 88, 96})
    {
        auto corrupt = bytes;
        std::fill_n(corrupt.begin() + offset, 4, 255);
        reject([&] { DecodeShaderBinary(corrupt); });
    }
    auto vertex = shader;
    vertex.Stage = ShaderStage::Vertex;
    auto geometry = shader;
    geometry.Stage = ShaderStage::Geometry;
    reject([&] { EncodeShaderBinary(geometry); });
    ShaderIdentity identity{};
    identity[0] = 42;
    identity[19] = 17;
    geometry.VertexIdentity = identity;
    const auto geometryBytes = EncodeShaderBinary(geometry);
    const auto decodedGeometry = DecodeShaderBinary(geometryBytes);
    require(decodedGeometry.VertexIdentity == identity);
    ValidateGeometryVertex(decodedGeometry, vertex, identity);
    auto wrongIdentity = identity;
    ++wrongIdentity[19];
    reject([&] { ValidateGeometryVertex(decodedGeometry, vertex, wrongIdentity); });
    auto wrongTarget = vertex;
    wrongTarget.Target = AgcTarget::Windows;
    reject([&] { ValidateGeometryVertex(decodedGeometry, wrongTarget, identity); });
    reject([&] { ValidateGeometryVertex(decodedGeometry, shader, identity); });
    vertex.VertexIdentity = identity;
    reject([&] { EncodeShaderBinary(vertex); });
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
