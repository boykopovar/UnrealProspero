#ifndef UNREALPROSPERO_RHI_LINEAR_MIP_LAYOUT_H
#define UNREALPROSPERO_RHI_LINEAR_MIP_LAYOUT_H

#include "CoreTypes.h"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace UnrealProspero::RHI
{
    struct LinearMipLayout
    {
        std::uint32_t Width;
        std::uint32_t Height;
        std::uint32_t Depth;
        std::size_t RowBytes;
        std::size_t RowPitch;
        std::size_t Offset;
        std::size_t Size;
        std::uint32_t RowCount;
    };

    UNREALPROSPERORHI_API std::vector<LinearMipLayout> CalculateLinearMipLayout(std::uint32_t width, std::uint32_t height, std::uint32_t depth, std::size_t blockBytes, std::uint32_t mipCount, std::uint32_t blockWidth = 1, std::uint32_t blockHeight = 1);
}

#endif
