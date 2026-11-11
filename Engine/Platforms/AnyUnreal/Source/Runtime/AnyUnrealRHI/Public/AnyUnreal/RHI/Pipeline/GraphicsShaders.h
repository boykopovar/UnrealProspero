#ifndef ANYUNREAL_RHI_GRAPHICS_SHADERS_H
#define ANYUNREAL_RHI_GRAPHICS_SHADERS_H

#include "AnyUnreal/RHI/Shader/GraphicsShaderLink.h"
#include "AnyUnreal/RHI/Shader/ShaderBindings.h"
#include "AnyUnreal/RHI/Descriptor/VertexBufferTable.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API GraphicsShaders final
    {
    public:
        GraphicsShaders(VertexShader& vertex, PixelShader* pixel, std::uint32_t primitiveType);
        VertexShader& GetVertexShader() const;
        PixelShader* GetPixelShader() const;
        void Record(CommandBuffer& commands, ShaderBindings& vertexBindings, ShaderBindings* pixelBindings, const VertexBufferTable& vertexBuffers, Shader::UserDataInputs vertexInputs = {}) const;

    private:
        TRefCountPtr<VertexShader> vertex;
        TRefCountPtr<PixelShader> pixel;
        GraphicsShaderLink link;
    };
}

#endif
