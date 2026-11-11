#include "AnyUnreal/RHI/Texture/DepthTexture2D.h"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    std::size_t DepthTexture2D::CalculateDataSize(std::uint32_t width, std::uint32_t height)
    {
        if (width == 0 || height == 0 || width > 16384 || height > 16384) throw std::out_of_range("Depth texture dimensions exceed the hardware range");
        return static_cast<std::size_t>((width + 127) / 128) * ((height + 127) / 128) * 65536;
    }

    DepthTexture2D::DepthTexture2D(std::uint32_t width, std::uint32_t height, std::uint32_t addressHigh) : width(width), height(height), storage(CalculateDataSize(width, height), addressHigh, 65536, AllocationUsage::Texture) {}
    std::array<std::uint32_t, 8> DepthTexture2D::GetDescriptor() const
    {
        const auto address = reinterpret_cast<std::uintptr_t>(storage.Data());
        if (address % 65536 != 0 || address >= (std::uint64_t{1} << 48)) throw std::runtime_error("Depth texture address cannot be encoded");
        return {
            static_cast<std::uint32_t>(address >> 8),
            static_cast<std::uint32_t>(address >> 40) | (22u << 20) | (((width - 1) & 3u) << 30),
            ((width - 1) >> 2) | ((height - 1) << 14),
            4u | (1u << 9) | (24u << 20) | (9u << 28),
            0, 0, 0, 0
        };
    }

    const BufferAllocation& DepthTexture2D::GetStorage() const { return storage; }
    std::uint32_t DepthTexture2D::GetWidth() const { return width; }
    std::uint32_t DepthTexture2D::GetHeight() const { return height; }
}
