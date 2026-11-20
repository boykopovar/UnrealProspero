#include "UnrealProspero/RHI/Shader/GeometryShader.h"
#include "UnrealProspero/RHI/Shader/ShaderResource.h"
#include "ShaderCore.h"
#include <limits>
#include <stdexcept>
#include <utility>

namespace UnrealProspero::RHI
{
    namespace
    {
        std::vector<std::uint8_t> readLibrary(TConstArrayView<uint8> source)
        {
            if (source.Num() < 4 || !source.GetData()) throw std::invalid_argument("Geometry shader library is empty or truncated");
            const FShaderCodeReader reader(source);
            const auto size = reader.GetActualShaderCodeSize();
            if (size <= 0 || size > source.Num()) throw std::invalid_argument("Geometry shader library has an invalid size");
            return {source.GetData(), source.GetData() + size};
        }

        std::unique_ptr<ShaderCode> createCode(std::span<const std::uint8_t> bytes, Shader::AgcTarget target)
        {
            if (bytes.size() > static_cast<std::size_t>(std::numeric_limits<int32>::max() - 4)) throw std::overflow_error("Geometry variant exceeds Unreal array limits");
            std::vector<std::uint8_t> source(bytes.begin(), bytes.end());
            source.insert(source.end(), {4, 0, 0, 0});
            return std::make_unique<ShaderCode>(TConstArrayView<uint8>(source.data(), static_cast<int32>(source.size())), Shader::ShaderStage::Geometry, target);
        }
    }

    struct GeometryShader::NativeVariant
    {
        std::unique_ptr<ShaderCode> Code;
        AgcShader Shader;

        NativeVariant(std::span<const std::uint8_t> bytes, Shader::AgcTarget target) : Code(createCode(bytes, target)), Shader(Code->GetBinary(), Shader::ShaderStage::Geometry, target) {}
    };

    GeometryShader::GeometryShader(TConstArrayView<uint8> source, const FShaderHash& hash, Shader::AgcTarget target) : storage(readLibrary(source)), variants(Shader::DecodeGeometryLibrary(storage)), defaultCode(createCode(variants.front().Bytes, target))
    {
        ShaderResourceTable = defaultCode->GetResourceTable();
        InitializeShaderMetadata(*this, source, hash);
    }

    GeometryShader::~GeometryShader() noexcept = default;

    const GeometryShader::NativeVariant& GeometryShader::getVariant(const ShaderCode& vertex) const
    {
        if (vertex.GetBinary().Stage != Shader::ShaderStage::Vertex || vertex.GetBinary().Target != defaultCode->GetBinary().Target) throw std::invalid_argument("Geometry variant requires a vertex shader for the same target");
        std::lock_guard lock(variantMutex);
        const auto& identity = vertex.GetIdentity();
        if (const auto found = nativeVariants.find(identity); found != nativeVariants.end()) return *found->second;
        for (const auto& variant : variants)
        {
            if (variant.Binary.VertexIdentity != identity) continue;
            auto native = std::make_unique<NativeVariant>(variant.Bytes, variant.Binary.Target);
            native->Code->ValidateVertex(vertex);
            const auto* result = native.get();
            nativeVariants.emplace(identity, std::move(native));
            return *result;
        }
        throw std::invalid_argument("Geometry shader has no offline variant for the requested vertex shader");
    }

    const ShaderCode& GeometryShader::GetCode() const { return *defaultCode; }
    const ShaderCode& GeometryShader::GetCode(const ShaderCode& vertex) const { return *getVariant(vertex).Code; }
    const AgcShader& GeometryShader::GetNativeShader(const ShaderCode& vertex) const { return getVariant(vertex).Shader; }
}
