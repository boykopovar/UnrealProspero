#ifndef ANYUNREAL_RHI_SHADER_RESOURCE_H
#define ANYUNREAL_RHI_SHADER_RESOURCE_H

#include "AnyUnreal/RHI/Shader/AgcShader.h"
#include "AnyUnreal/RHI/Shader/ShaderCode.h"
#include <type_traits>

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API void InitializeShaderMetadata(FRHIShader& resource, const ShaderCode& code, const FShaderHash& hash);

    template<typename TBase, Shader::ShaderStage TStage>
    class ShaderResource final : public TBase
    {
        static_assert((std::is_same_v<TBase, FRHIComputeShader> && TStage == Shader::ShaderStage::Compute) || (std::is_same_v<TBase, FRHIVertexShader> && TStage == Shader::ShaderStage::Vertex) || (std::is_same_v<TBase, FRHIPixelShader> && TStage == Shader::ShaderStage::Pixel) || (std::is_same_v<TBase, FRHIGeometryShader> && TStage == Shader::ShaderStage::Geometry));

    public:
        ShaderResource(TConstArrayView<uint8> source, const FShaderHash& hash, Shader::AgcTarget target) : code(source, TStage, target), shader(code.GetBinary(), TStage, target)
        {
            this->ShaderResourceTable = code.GetResourceTable();
            InitializeShaderMetadata(*this, code, hash);
        }

        ~ShaderResource() noexcept override = default;
        const AgcShader& GetNativeShader() const { return shader; }
        const ShaderCode& GetCode() const { return code; }

    private:
        ShaderCode code;
        AgcShader shader;
    };

    extern template class ANYUNREALRHI_API ShaderResource<FRHIComputeShader, Shader::ShaderStage::Compute>;
    extern template class ANYUNREALRHI_API ShaderResource<FRHIVertexShader, Shader::ShaderStage::Vertex>;
    extern template class ANYUNREALRHI_API ShaderResource<FRHIPixelShader, Shader::ShaderStage::Pixel>;
    extern template class ANYUNREALRHI_API ShaderResource<FRHIGeometryShader, Shader::ShaderStage::Geometry>;
}

#endif
