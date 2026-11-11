#ifndef UNREALPROSPERO_RHI_GRAPHICS_SHADER_LINK_H
#define UNREALPROSPERO_RHI_GRAPHICS_SHADER_LINK_H

#include "UnrealProspero/RHI/Shader/AgcShader.h"
#include "SceTypes.hpp"
#include <array>

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API GraphicsShaderLink final
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
