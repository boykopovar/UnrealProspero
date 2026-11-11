#include "AnyUnreal/RHI/Command/Rasterization.h"
#include "prx/libSceAgc/DcbState/include/Registers.hpp"

namespace AnyUnreal::RHI
{
    void WriteRasterizationRegisters(CommandBuffer& commands, const RasterizerState& state, uint32 modeControl, uint32 clipControl, bool depthBias)
    {
        const auto mode = state.UpdateModeControl(modeControl, depthBias);
        const auto clip = state.UpdateClipControl(clipControl);
        sceAgcDcbSetCxRegisterDirect(&commands, {0x205, mode});
        sceAgcDcbSetCxRegisterDirect(&commands, {0x204, clip});
    }
}
