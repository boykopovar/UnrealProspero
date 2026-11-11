#ifndef ANYUNREAL_RHI_GRAPHICS_SHADERS_H
#define ANYUNREAL_RHI_GRAPHICS_SHADERS_H

#include "AnyUnreal/RHI/Shader/GraphicsShaderLink.h"
#include "AnyUnreal/RHI/Shader/ShaderBindings.h"
#include "AnyUnreal/RHI/Descriptor/VertexBufferTable.h"
#include "AnyUnreal/RHI/Descriptor/MergedTables.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API GraphicsShaders final
    {
    public:
        GraphicsShaders(VertexShader& vertex, PixelShader* pixel, GeometryShader* geometry, std::uint32_t primitiveType);
        VertexShader& GetVertexShader() const;
        PixelShader* GetPixelShader() const;
        GeometryShader* GetGeometryShader() const;
        const Shader::UserDataLayout& GetVertexInputLayout() const;
        std::unique_ptr<MergedDescriptorTables> Record(CommandBuffer& commands, ShaderBindings& vertexBindings, ShaderBindings* pixelBindings, ShaderBindings* geometryBindings, const VertexBufferTable& vertexBuffers, Shader::UserDataInputs vertexInputs = {}) const;

    private:
        TRefCountPtr<VertexShader> vertex;
        TRefCountPtr<PixelShader> pixel;
        TRefCountPtr<GeometryShader> geometry;
        GraphicsShaderLink link;
    };
}

#endif
