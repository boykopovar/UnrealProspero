#include "UnrealProspero/RHI/Texture/DepthTexture2D.h"
#include "UnrealProspero/RHI/Device/DeviceLimits.h"
#include <stdexcept>

namespace UnrealProspero::RHI
{
    std::size_t DepthTexture2D::CalculateDataSize(std::uint32_t width, std::uint32_t height, bool stencil)
    {
        if (width == 0 || height == 0 || width > DeviceLimits::Texture2D || height > DeviceLimits::Texture2D) throw std::out_of_range("Depth texture dimensions exceed the hardware range");
        const auto depthBytes = static_cast<std::size_t>((width + 127) / 128) * ((height + 127) / 128) * 65536;
        const auto stencilBytes = stencil ? static_cast<std::size_t>((width + 255) / 256) * ((height + 255) / 256) * 65536 : 0;
        return depthBytes + stencilBytes;
    }

    DepthTexture2D::DepthTexture2D(std::uint32_t width, std::uint32_t height, std::uint32_t addressHigh, bool stencil) : width(width), height(height), stencilEnabled(stencil), storage(CalculateDataSize(width, height, stencil), addressHigh, 65536, AllocationUsage::Texture) {}
    std::array<std::uint32_t, 8> DepthTexture2D::GetDescriptor(bool stencil) const
    {
        const auto address = reinterpret_cast<std::uintptr_t>((stencil ? GetStencilRange() : GetDepthRange()).data());
        if (address % 65536 != 0 || address >= (std::uint64_t{1} << 48)) throw std::runtime_error("Depth texture address cannot be encoded");
        return {
            static_cast<std::uint32_t>(address >> 8),
            static_cast<std::uint32_t>(address >> 40) | ((stencil ? 5u : 22u) << 20) | (((width - 1) & 3u) << 30),
            ((width - 1) >> 2) | ((height - 1) << 14),
            4u | (stencil ? 4u << 3 : 0u) | (1u << 9) | (24u << 20) | (9u << 28),
            0, 0, 0, 0
        };
    }

    bool DepthTexture2D::HasStencil() const { return stencilEnabled; }
    std::span<std::byte> DepthTexture2D::GetDepthRange() const { return storage.GetRange(0, CalculateDataSize(width, height)); }
    std::span<std::byte> DepthTexture2D::GetStencilRange() const
    {
        if (!stencilEnabled) throw std::logic_error("Depth texture has no stencil plane");
        const auto offset = CalculateDataSize(width, height);
        return storage.GetRange(offset, storage.Size() - offset);
    }

    const BufferAllocation& DepthTexture2D::GetStorage() const { return storage; }
    std::uint32_t DepthTexture2D::GetWidth() const { return width; }
    std::uint32_t DepthTexture2D::GetHeight() const { return height; }
}
