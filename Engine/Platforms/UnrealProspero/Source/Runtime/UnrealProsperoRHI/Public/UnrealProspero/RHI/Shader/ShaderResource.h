#ifndef UNREALPROSPERO_RHI_SHADER_RESOURCE_H
#define UNREALPROSPERO_RHI_SHADER_RESOURCE_H

#include "UnrealProspero/RHI/Shader/AgcShader.h"
#include "UnrealProspero/RHI/Shader/ShaderCode.h"
#include <type_traits>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API void InitializeShaderMetadata(FRHIShader& resource, const ShaderCode& code, const FShaderHash& hash);

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
        const AgcShader& GetNativeShader(std::uint32_t colorExportFormat) const requires(TStage == Shader::ShaderStage::Pixel)
        {
            const auto& binary = code.GetBinary();
            if (colorExportFormat == binary.ColorExportFormat) return shader;
            std::lock_guard lock(colorExportMutex);
            if (const auto found = colorExportShaders.find(colorExportFormat); found != colorExportShaders.end()) return *found->second;
            for (const auto& variant : binary.ColorExports)
            {
                if (variant.Format != colorExportFormat) continue;
                const auto layout = Shader::UserDataLayout::Read(variant.Metadata, TStage);
                if (!(layout == code.GetUserData())) throw std::invalid_argument("Pixel shader export variants require identical user data layouts");
                auto selected = binary;
                selected.Header = variant.Header;
                selected.Code = variant.Code;
                selected.Metadata = variant.Metadata;
                auto native = std::make_unique<AgcShader>(selected, TStage, binary.Target);
                const auto* result = native.get();
                colorExportShaders.emplace(colorExportFormat, std::move(native));
                return *result;
            }
            throw std::invalid_argument("Pixel shader has no offline variant for the requested color export format");
        }
        const ShaderCode& GetCode() const { return code; }

    private:
        ShaderCode code;
        AgcShader shader;
        mutable std::mutex colorExportMutex;
        mutable std::map<std::uint32_t, std::unique_ptr<AgcShader>> colorExportShaders;
    };

    extern template class UNREALPROSPERORHI_API ShaderResource<FRHIComputeShader, Shader::ShaderStage::Compute>;
    extern template class UNREALPROSPERORHI_API ShaderResource<FRHIVertexShader, Shader::ShaderStage::Vertex>;
    extern template class UNREALPROSPERORHI_API ShaderResource<FRHIPixelShader, Shader::ShaderStage::Pixel>;
    extern template class UNREALPROSPERORHI_API ShaderResource<FRHIGeometryShader, Shader::ShaderStage::Geometry>;
}

#endif
