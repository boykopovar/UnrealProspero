#include "AnyUnreal/RHI/Command/DepthBounds.h"
#include "prx/libSceAgc/DcbState/include/Registers.hpp"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace AnyUnreal::RHI
{
    void WriteDepthBounds(CommandBuffer& commands, float minDepth, float maxDepth)
    {
        if (!std::isfinite(minDepth) || !std::isfinite(maxDepth) || minDepth < 0 || minDepth > 1 || maxDepth < 0 || maxDepth > 1) throw std::invalid_argument("Depth bounds must be finite values in the normalized depth range");
        sceAgcDcbSetCxRegisterDirect(&commands, {8, std::bit_cast<std::uint32_t>(minDepth)});
        sceAgcDcbSetCxRegisterDirect(&commands, {9, std::bit_cast<std::uint32_t>(maxDepth)});
    }
}
