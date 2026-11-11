#include "AnyUnreal/RHI/Texture/LinearTexture2D.h"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    namespace
    {
        struct Format
        {
            std::uint32_t Code;
            std::uint32_t Bytes;
            std::uint32_t Swizzle;
        };

        Format describe(TextureFormat format)
        {
            switch (format)
            {
            case TextureFormat::R8Unorm: return {1, 1, 4 | (1 << 9)};
            case TextureFormat::Rg8Unorm: return {14, 2, 4 | (5 << 3) | (1 << 9)};
            case TextureFormat::Rgba8Unorm: return {56, 4, 0xfac};
            case TextureFormat::Rgba8Srgb: return {130, 4, 0xfac};
            case TextureFormat::Bgra8Unorm: return {56, 4, 6 | (5 << 3) | (4 << 6) | (7 << 9)};
            case TextureFormat::Bgra8Srgb: return {130, 4, 6 | (5 << 3) | (4 << 6) | (7 << 9)};
            case TextureFormat::R32Float: return {22, 4, 4 | (1 << 9)};
            case TextureFormat::Rgba32Float: return {77, 16, 0xfac};
            default: throw std::invalid_argument("Unknown linear texture format");
            }
        }

        std::uint32_t dimension(std::uint32_t value)
        {
            if (value == 0 || value > 16384) throw std::out_of_range("Texture dimension exceeds the hardware range");
            return value;
        }
    }

    LinearTexture2D::LinearTexture2D(std::uint32_t width, std::uint32_t height, TextureFormat format, std::uint32_t addressHigh) : width(dimension(width)), height(dimension(height)), format(format), rowBytes(this->width * describe(format).Bytes), rowPitch((rowBytes + 255) & ~std::size_t{255}), storage(rowPitch * this->height, addressHigh)
    {
        const auto address = reinterpret_cast<std::uintptr_t>(storage.Data());
        if (address % 256 != 0 || address >= (std::uint64_t{1} << 48)) throw std::runtime_error("Texture storage address cannot be encoded");
        const auto info = describe(format);
        descriptor = {
            static_cast<std::uint32_t>(address >> 8),
            static_cast<std::uint32_t>(address >> 40) | (info.Code << 20) | (((width - 1) & 3) << 30),
            ((width - 1) >> 2) | ((height - 1) << 14),
            info.Swizzle | (9u << 28),
            0, 0, 0, 0
        };
    }

    void LinearTexture2D::Upload(std::span<const std::byte> source, std::size_t sourcePitch)
    {
        if (!source.data() || sourcePitch < rowBytes || source.size() < rowBytes || (height - 1) > (source.size() - rowBytes) / sourcePitch) throw std::invalid_argument("Texture upload does not contain all source rows");
        for (std::size_t row = 0; row < height; ++row) storage.Upload(row * rowPitch, source.subspan(row * sourcePitch, rowBytes));
    }

    const std::array<std::uint32_t, 8>& LinearTexture2D::GetDescriptor() const { return descriptor; }
    std::array<std::uint32_t, 8> LinearTexture2D::CreateViewDescriptor(bool disableSrgb) const
    {
        auto result = descriptor;
        if (disableSrgb && ((result[1] >> 20) & 0x1ffu) == describe(TextureFormat::Rgba8Srgb).Code) result[1] = (result[1] & ~(0x1ffu << 20)) | (describe(TextureFormat::Rgba8Unorm).Code << 20);
        return result;
    }

    const BufferAllocation& LinearTexture2D::GetStorage() const { return storage; }
    std::size_t LinearTexture2D::GetRowPitch() const { return rowPitch; }
    std::size_t LinearTexture2D::GetRowBytes() const { return rowBytes; }
    std::uint32_t LinearTexture2D::GetWidth() const { return width; }
    std::uint32_t LinearTexture2D::GetHeight() const { return height; }
    TextureFormat LinearTexture2D::GetFormat() const { return format; }
}
