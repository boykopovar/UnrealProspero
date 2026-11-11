#include "UnrealProspero/RHI/Texture/LinearMipLayout.h"
#include "UnrealProspero/RHI/Device/DeviceLimits.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace UnrealProspero::RHI
{
    std::vector<LinearMipLayout> CalculateLinearMipLayout(std::uint32_t width, std::uint32_t height, std::uint32_t depth, std::size_t blockBytes, std::uint32_t mipCount, std::uint32_t blockWidth, std::uint32_t blockHeight)
    {
        if (width == 0 || height == 0 || depth == 0 || std::max({width, height, depth}) > DeviceLimits::Texture2D) throw std::out_of_range("Linear texture dimensions exceed the hardware range");
        if (blockBytes > 16 || !std::has_single_bit(blockBytes)) throw std::invalid_argument("Invalid linear texture block size");
        if ((blockWidth != 1 || blockHeight != 1) && (blockWidth != 4 || blockHeight != 4)) throw std::invalid_argument("Unsupported linear texture block dimensions");
        if (blockWidth == 4 && blockBytes != 8 && blockBytes != 16) throw std::invalid_argument("Compressed texture blocks must contain 8 or 16 bytes");
        if (mipCount == 0 || mipCount > static_cast<std::uint32_t>(std::bit_width(std::max({width, height, depth})))) throw std::invalid_argument("Texture mip count exceeds its dimensions");
        std::vector<LinearMipLayout> levels(mipCount);
        std::size_t offset = 0;
        const auto blocksWidth = (width + blockWidth - 1) / blockWidth;
        const auto blocksHeight = (height + blockHeight - 1) / blockHeight;
        for (auto level = mipCount; level-- != 0;)
        {
            const auto divisor = std::uint32_t{1} << level;
            const auto paddedWidth = (blocksWidth + divisor - 1) / divisor;
            const auto paddedHeight = (blocksHeight + divisor - 1) / divisor;
            const auto rowPitch = (paddedWidth * blockBytes + 255) & ~std::size_t{255};
            const auto size = rowPitch * paddedHeight;
            const auto mipWidth = std::max(width >> level, 1u);
            const auto mipHeight = std::max(height >> level, 1u);
            const auto rowBytes = ((mipWidth + blockWidth - 1) / blockWidth) * blockBytes;
            const auto rowCount = (mipHeight + blockHeight - 1) / blockHeight;
            levels[level] = {mipWidth, mipHeight, std::max(depth >> level, 1u), rowBytes, rowPitch, offset, size, rowCount};
            offset += size;
        }
        return levels;
    }
}
