#include "AnyUnreal/RHI/Texture/DepthTexture2D.h"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    std::size_t DepthTexture2D::CalculateDataSize(std::uint32_t width, std::uint32_t height)
    {
        if (width == 0 || height == 0 || width > 16384 || height > 16384) throw std::out_of_range("Depth texture dimensions exceed the hardware range");
        return static_cast<std::size_t>((width + 127) / 128) * ((height + 127) / 128) * 65536;
    }

    DepthTexture2D::DepthTexture2D(std::uint32_t width, std::uint32_t height, std::uint32_t addressHigh) : width(width), height(height), storage(CalculateDataSize(width, height), addressHigh, 65536) {}
    const BufferAllocation& DepthTexture2D::GetStorage() const { return storage; }
    std::uint32_t DepthTexture2D::GetWidth() const { return width; }
    std::uint32_t DepthTexture2D::GetHeight() const { return height; }
}
