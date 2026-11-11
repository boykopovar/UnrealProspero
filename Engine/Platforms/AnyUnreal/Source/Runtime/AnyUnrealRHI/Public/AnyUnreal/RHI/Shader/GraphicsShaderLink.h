#ifndef ANYUNREAL_RHI_GRAPHICS_SHADER_LINK_H
#define ANYUNREAL_RHI_GRAPHICS_SHADER_LINK_H

#include "AnyUnreal/RHI/Shader/AgcShader.h"
#include "SceTypes.hpp"
#include <array>

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API GraphicsShaderLink final
    {
    public:
        GraphicsShaderLink(const AgcShader& vertex, const AgcShader* pixel, std::uint32_t primitiveType);
        void Record(CommandBuffer& commands) const;

    private:
        std::array<ShaderRegister, 34> context{};
        std::array<ShaderRegister, 3> primitive{};
    };
}

#endif
