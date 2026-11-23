#include "UnrealProspero/RHI/Command/SampleCoverage.h"
#include "prx/libSceAgc/DcbState/include/Registers.hpp"
#include <array>
#include <stdexcept>

namespace UnrealProspero::RHI
{
    void WriteSampleCoverage(CommandBuffer& commands, std::uint32_t samples)
    {
        if (samples != 1) throw std::runtime_error("Multisample raster coverage is not implemented");
        const std::array registers{
            ShaderRegister{0x201, (1u << 16) | (1u << 17) | (1u << 20)},
            ShaderRegister{0x2dc, 0},
            ShaderRegister{0x2f8, 0},
            ShaderRegister{0x30e, 0xffffffffu},
            ShaderRegister{0x30f, 0xffffffffu}
        };
        for (const auto value : registers) sceAgcDcbSetCxRegisterDirect(&commands, value);
    }
}
