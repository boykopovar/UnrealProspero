#include "AnyUnreal/RHI/Command/DepthStencil.h"
#include "prx/libSceAgc/DcbState/include/Registers.hpp"
#include <array>

namespace AnyUnreal::RHI
{
    void WriteDepthStencilRegisters(CommandBuffer& commands, const DepthStencilState& state, uint32 reference, bool hasDepth, bool hasStencil, bool depthBounds)
    {
        const auto referenceMask = state.GetStencilReferenceMask(reference);
        const std::array registers{
            ShaderRegister{0x200, state.GetDepthControl(hasDepth, hasStencil, depthBounds)},
            ShaderRegister{0x10b, state.GetStencilControl()},
            ShaderRegister{0x10c, referenceMask},
            ShaderRegister{0x10d, referenceMask}
        };
        for (const auto& value : registers) sceAgcDcbSetCxRegisterDirect(&commands, value);
    }
}
