#include "AnyUnreal/RHI/Texture/LinearTexture2D.h"
#include <stdexcept>
#include <cstring>

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
            case TextureFormat::R16Float: return {13, 2, 4 | (1 << 9)};
            case TextureFormat::Rg16Float: return {29, 4, 4 | (5 << 3) | (1 << 9)};
            case TextureFormat::R32Float: return {22, 4, 4 | (1 << 9)};
            case TextureFormat::Rgba16Float: return {71, 8, 0xfac};
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

    std::size_t LinearTexture2D::CalculateRowBytes(std::uint32_t width, TextureFormat format)
    {
        return dimension(width) * describe(format).Bytes;
    }

    std::size_t LinearTexture2D::CalculateRowPitch(std::uint32_t width, TextureFormat format)
    {
        return (CalculateRowBytes(width, format) + 255) & ~std::size_t{255};
    }

    std::size_t LinearTexture2D::CalculateDataSize(std::uint32_t width, std::uint32_t height, TextureFormat format)
    {
        return CalculateRowPitch(width, format) * dimension(height);
    }

    LinearTexture2D::LinearTexture2D(std::uint32_t width, std::uint32_t height, TextureFormat format, std::uint32_t addressHigh, std::size_t alignment) : width(dimension(width)), height(dimension(height)), format(format), rowBytes(CalculateRowBytes(width, format)), rowPitch(CalculateRowPitch(width, format)), storage(rowPitch * this->height, addressHigh, alignment, AllocationUsage::Texture)
    {
        descriptor = CreateDescriptor(width, height, format, storage.Data());
    }

    std::array<std::uint32_t, 8> LinearTexture2D::CreateDescriptor(std::uint32_t width, std::uint32_t height, TextureFormat format, const void* data, bool disableSrgb)
    {
        dimension(width);
        dimension(height);
        const auto address = reinterpret_cast<std::uintptr_t>(data);
        if (address == 0 || address % 256 != 0 || address >= (std::uint64_t{1} << 48)) throw std::runtime_error("Texture storage address cannot be encoded");
        if (disableSrgb && format == TextureFormat::Rgba8Srgb) format = TextureFormat::Rgba8Unorm;
        if (disableSrgb && format == TextureFormat::Bgra8Srgb) format = TextureFormat::Bgra8Unorm;
        const auto info = describe(format);
        return {
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

    void LinearTexture2D::CopyRowsTo(std::span<std::byte> destination, std::size_t destinationPitch) const
    {
        if (!destination.data() || destinationPitch < rowBytes || destination.size() < rowBytes || (height - 1) > (destination.size() - rowBytes) / destinationPitch) throw std::invalid_argument("Texture read destination does not contain all rows");
        const auto sourceAddress = reinterpret_cast<std::uintptr_t>(storage.Data());
        const auto destinationAddress = reinterpret_cast<std::uintptr_t>(destination.data());
        if ((destinationAddress >= sourceAddress && destinationAddress - sourceAddress < storage.Size()) || (sourceAddress >= destinationAddress && sourceAddress - destinationAddress < destination.size())) throw std::invalid_argument("Texture read destination overlaps its storage");
        const auto* source = static_cast<const std::byte*>(storage.Data());
        for (std::size_t row = 0; row < height; ++row) std::memcpy(destination.data() + row * destinationPitch, source + row * rowPitch, rowBytes);
    }

    const std::array<std::uint32_t, 8>& LinearTexture2D::GetDescriptor() const { return descriptor; }
    std::array<std::uint32_t, 8> LinearTexture2D::CreateViewDescriptor(bool disableSrgb) const
    {
        return CreateDescriptor(width, height, format, storage.Data(), disableSrgb);
    }

    const BufferAllocation& LinearTexture2D::GetStorage() const { return storage; }
    std::size_t LinearTexture2D::GetRowPitch() const { return rowPitch; }
    std::size_t LinearTexture2D::GetRowBytes() const { return rowBytes; }
    std::uint32_t LinearTexture2D::GetWidth() const { return width; }
    std::uint32_t LinearTexture2D::GetHeight() const { return height; }
    TextureFormat LinearTexture2D::GetFormat() const { return format; }
}
