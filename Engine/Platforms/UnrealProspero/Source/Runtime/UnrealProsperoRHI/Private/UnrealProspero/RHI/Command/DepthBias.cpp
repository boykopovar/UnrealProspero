#include "UnrealProspero/RHI/Command/DepthBias.h"
#include "prx/libSceAgc/DcbState/include/Registers.hpp"
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace UnrealProspero::RHI
{
    void WriteDepthBias(CommandBuffer& commands, float constant, float slope)
    {
        const auto scale = slope * 16.0f;
        if (!std::isfinite(constant) || !std::isfinite(scale)) throw std::invalid_argument("Depth bias must be finite and representable");
        const auto offset = std::bit_cast<std::uint32_t>(constant);
        const auto gradient = std::bit_cast<std::uint32_t>(scale);
        const std::array registers{
            ShaderRegister{0x2de, 0x1e9}, ShaderRegister{0x2df, 0},
            ShaderRegister{0x2e0, gradient}, ShaderRegister{0x2e1, offset},
            ShaderRegister{0x2e2, gradient}, ShaderRegister{0x2e3, offset}
        };
        for (const auto value : registers) sceAgcDcbSetCxRegisterDirect(&commands, value);
    }
}
