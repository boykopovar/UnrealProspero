#ifndef UNREALPROSPERO_RHI_GEOMETRY_SHADER_H
#define UNREALPROSPERO_RHI_GEOMETRY_SHADER_H

#include "UnrealProspero/RHI/Shader/AgcShader.h"
#include "UnrealProspero/RHI/Shader/ShaderCode.h"
#include "UnrealProspero/Shader/GeometryLibrary.h"
#include <map>
#include <memory>
#include <mutex>

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API GeometryShader final : public FRHIGeometryShader
    {
    public:
        GeometryShader(TConstArrayView<uint8> source, const FShaderHash& hash, Shader::AgcTarget target);
        ~GeometryShader() noexcept override;
        const ShaderCode& GetCode() const;
        const ShaderCode& GetCode(const ShaderCode& vertex) const;
        const AgcShader& GetNativeShader(const ShaderCode& vertex) const;

    private:
        struct NativeVariant;
        const NativeVariant& getVariant(const ShaderCode& vertex) const;
        std::vector<std::uint8_t> storage;
        std::vector<Shader::GeometryVariantView> variants;
        std::unique_ptr<ShaderCode> defaultCode;
        mutable std::mutex variantMutex;
        mutable std::map<Shader::ShaderIdentity, std::unique_ptr<NativeVariant>> nativeVariants;
    };
}

#endif
