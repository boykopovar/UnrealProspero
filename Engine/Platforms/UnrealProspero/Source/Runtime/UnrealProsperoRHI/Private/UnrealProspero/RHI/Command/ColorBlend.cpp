#include "UnrealProspero/RHI/Command/ColorBlend.h"
#include "prx/libSceAgc/DcbState/include/Registers.hpp"
#include <array>

namespace UnrealProspero::RHI
{
    void WriteColorBlendRegisters(CommandBuffer& commands, const BlendState& state, uint32 targetCount)
    {
        const auto mask = state.GetTargetMask(targetCount);
        std::array<ShaderRegister, MaxSimultaneousRenderTargets + 1> registers;
        for (uint32 index = 0; index < MaxSimultaneousRenderTargets; ++index) registers[index] = {0x1e0u + index, index < targetCount ? state.GetControl(index) : 0};
        registers.back() = {0x8e, mask};
        for (const auto& value : registers) sceAgcDcbSetCxRegisterDirect(&commands, value);
    }
}
