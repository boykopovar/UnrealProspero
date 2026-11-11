#include "AnyUnreal/RHI/Shader/GraphicsShaderLink.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libSceAgc/Shader/include/LinkShaders.hpp"
#include "prx/libSceAgc/DcbState/include/Registers.hpp"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    GraphicsShaderLink::GraphicsShaderLink(const AgcShader& vertex, const AgcShader* pixel, std::uint32_t primitiveType)
    {
        if (vertex.Get().type != 2 || (pixel && pixel->Get().type != 1)) throw std::invalid_argument("Graphics link requires vertex and optional pixel shaders");
        Kernel::CheckResult(sceAgcLinkShaders(context.data(), primitive.data(), nullptr, &vertex.Get(), pixel ? &pixel->Get() : nullptr, primitiveType), "sceAgcLinkShaders");
    }

    void GraphicsShaderLink::Record(CommandBuffer& commands) const
    {
        for (const auto& value : context) sceAgcDcbSetCxRegisterDirect(&commands, value);
        for (const auto& value : primitive) sceAgcDcbSetUcRegisterDirect(&commands, value);
    }
}
