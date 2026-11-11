#ifndef ANYUNREAL_RHI_LINEAR_MIP_LAYOUT_H
#define ANYUNREAL_RHI_LINEAR_MIP_LAYOUT_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace AnyUnreal::RHI
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
    };

    ANYUNREALRHI_API std::vector<LinearMipLayout> CalculateLinearMipLayout(std::uint32_t width, std::uint32_t height, std::uint32_t depth, std::size_t pixelBytes, std::uint32_t mipCount);
}

#endif
