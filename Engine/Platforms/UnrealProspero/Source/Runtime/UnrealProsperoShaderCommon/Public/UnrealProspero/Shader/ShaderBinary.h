#ifndef UNREALPROSPERO_SHADER_BINARY_H
#define UNREALPROSPERO_SHADER_BINARY_H

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace UnrealProspero::Shader
{
    enum class AgcTarget : std::uint32_t
    {
        Windows,
        Linux,
        Ps5
    };

    enum class ShaderStage : std::uint32_t
    {
        Vertex,
        Pixel,
        Compute,
        Geometry
    };

    enum class ResourceKind : std::uint32_t
    {
        UniformBuffer,
        Sampler,
        ShaderResource,
        UnorderedAccess,
        CombinedImageSampler
    };

    struct RuntimeBinding
    {
        ResourceKind Kind;
        std::uint32_t ParameterIndex;
        std::uint32_t Set;
        std::uint32_t Binding;
        std::uint32_t Count;
        std::uint32_t Offset;
        std::uint32_t Stride;
        std::uint32_t BlockSize;
        std::string Type;
    };

    using ShaderIdentity = std::array<std::uint8_t, 20>;

    struct ColorExportVariantView
    {
        std::uint32_t Format;
        std::span<const std::uint8_t> Header;
        std::span<const std::uint8_t> Code;
        std::span<const std::uint8_t> Metadata;
    };

    struct ShaderBinaryView
    {
        ShaderStage Stage;
        AgcTarget Target;
        std::span<const std::uint8_t> Header;
        std::span<const std::uint8_t> Code;
        std::span<const std::uint8_t> Metadata;
        std::span<const std::uint8_t> ResourceTable;
        std::vector<RuntimeBinding> Bindings;
        ShaderIdentity VertexIdentity{};
        std::uint32_t ColorExportFormat = 0;
        std::vector<ColorExportVariantView> ColorExports;
    };

    UNREALPROSPEROSHADERCOMMON_API void ValidateGeometryVertex(const ShaderBinaryView& geometry, const ShaderBinaryView& vertex, const ShaderIdentity& identity);
    UNREALPROSPEROSHADERCOMMON_API void ValidateRuntimeBindings(std::span<const RuntimeBinding> bindings);
    UNREALPROSPEROSHADERCOMMON_API std::vector<std::uint8_t> EncodeShaderBinary(const ShaderBinaryView& shader);
    UNREALPROSPEROSHADERCOMMON_API ShaderBinaryView DecodeShaderBinary(std::span<const std::uint8_t> bytes);
    UNREALPROSPEROSHADERCOMMON_API ShaderBinaryView DecodeUnrealShaderCode(std::span<const std::uint8_t> bytes);
}

#endif
