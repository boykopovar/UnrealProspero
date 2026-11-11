#include "AnyUnreal/RHI/Texture/LinearMipLayout.h"
#include "AnyUnreal/RHI/Device/DeviceLimits.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace AnyUnreal::RHI
{
    std::vector<LinearMipLayout> CalculateLinearMipLayout(std::uint32_t width, std::uint32_t height, std::uint32_t depth, std::size_t pixelBytes, std::uint32_t mipCount)
    {
        if (width == 0 || height == 0 || depth == 0 || std::max({width, height, depth}) > DeviceLimits::Texture2D) throw std::out_of_range("Linear texture dimensions exceed the hardware range");
        if (pixelBytes > 16 || !std::has_single_bit(pixelBytes)) throw std::invalid_argument("Invalid linear texture pixel size");
        if (mipCount == 0 || mipCount > static_cast<std::uint32_t>(std::bit_width(std::max({width, height, depth})))) throw std::invalid_argument("Texture mip count exceeds its dimensions");
        std::vector<LinearMipLayout> levels(mipCount);
        std::size_t offset = 0;
        for (auto level = mipCount; level-- != 0;)
        {
            const auto divisor = std::uint32_t{1} << level;
            const auto paddedWidth = (width + divisor - 1) / divisor;
            const auto paddedHeight = (height + divisor - 1) / divisor;
            const auto rowPitch = (paddedWidth * pixelBytes + 255) & ~std::size_t{255};
            const auto size = rowPitch * paddedHeight;
            const auto mipWidth = std::max(width >> level, 1u);
            levels[level] = {mipWidth, std::max(height >> level, 1u), std::max(depth >> level, 1u), mipWidth * pixelBytes, rowPitch, offset, size};
            offset += size;
        }
        return levels;
    }
}
