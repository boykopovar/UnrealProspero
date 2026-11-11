#include "UnrealProspero/RHI/Command/TextureCopy.h"
#include "UnrealProspero/RHI/Command/BufferCopy.h"
#include "UnrealProspero/RHI/Command/MemoryAcquire.h"
#include <stdexcept>
#include <algorithm>

namespace UnrealProspero::RHI
{
    namespace
    {
        TextureFormat storageFormat(TextureFormat format)
        {
            if (format == TextureFormat::Rgba8Srgb) return TextureFormat::Rgba8Unorm;
            if (format == TextureFormat::Bgra8Srgb) return TextureFormat::Bgra8Unorm;
            return format;
        }
    }

    void ValidateTextureCopy(const LinearTexture3D& destination, const LinearTexture3D& source, const VolumeCopyRegion& region)
    {
        if (storageFormat(destination.GetFormat()) != storageFormat(source.GetFormat())) throw std::invalid_argument("Volume copy formats are incompatible");
        const auto& sourceMip = source.GetMipLayout(region.SourceMip);
        const auto& destinationMip = destination.GetMipLayout(region.DestinationMip);
        const std::array sourceSize{sourceMip.Width, sourceMip.Height, sourceMip.Depth};
        const std::array destinationSize{destinationMip.Width, destinationMip.Height, destinationMip.Depth};
        bool overlap = source.GetStorage().Data() == destination.GetStorage().Data() && region.SourceMip == region.DestinationMip;
        for (std::size_t axis = 0; axis < 3; ++axis)
        {
            const auto size = region.Extent[axis];
            if (size == 0) throw std::invalid_argument("Volume copy requires a nonempty region");
            if (region.Source[axis] >= sourceSize[axis] || size > sourceSize[axis] - region.Source[axis]) throw std::out_of_range("Volume copy exceeds its source");
            if (region.Destination[axis] >= destinationSize[axis] || size > destinationSize[axis] - region.Destination[axis]) throw std::out_of_range("Volume copy exceeds its destination");
            overlap = overlap && region.Source[axis] < region.Destination[axis] + size && region.Destination[axis] < region.Source[axis] + size;
        }
        if (overlap) throw std::invalid_argument("Volume copy regions overlap");
    }

    void WriteTextureCopy(CommandBuffer& commands, const LinearTexture3D& destination, const LinearTexture3D& source, const VolumeCopyRegion& region)
    {
        ValidateTextureCopy(destination, source, region);
        const auto& sourceMip = source.GetMipLayout(region.SourceMip);
        const auto& destinationMip = destination.GetMipLayout(region.DestinationMip);
        const auto pixelBytes = LinearTexture2D::CalculateRowBytes(1, source.GetFormat());
        const auto rowBytes = region.Extent[0] * pixelBytes;
        const auto& sourceStorage = source.GetStorage();
        const auto& destinationStorage = destination.GetStorage();
        const GpuMemoryRange ranges[]{{sourceStorage.Data(), sourceStorage.Size()}, {destinationStorage.Data(), destinationStorage.Size()}};
        WriteMemoryAcquire(commands, ranges);
        for (std::uint32_t z = 0; z < region.Extent[2]; ++z)
        {
            for (std::uint32_t y = 0; y < region.Extent[1]; ++y)
            {
                const auto from = sourceMip.Offset + (region.Source[2] + z) * source.GetSlicePitch() + (region.Source[1] + y) * sourceMip.RowPitch + region.Source[0] * pixelBytes;
                const auto to = destinationMip.Offset + (region.Destination[2] + z) * destination.GetSlicePitch() + (region.Destination[1] + y) * destinationMip.RowPitch + region.Destination[0] * pixelBytes;
                WriteBufferCopy(commands, destinationStorage.GetRange(to, rowBytes).data(), sourceStorage.GetRange(from, rowBytes).data(), rowBytes);
            }
        }
    }

    void ValidateTextureCopy(const LinearTexture2D& destination, const LinearTexture2D& source, const TextureCopyRegion& region)
    {
        if (region.SourceLayer >= source.GetArrayLayers() || region.DestinationLayer >= destination.GetArrayLayers()) throw std::out_of_range("Texture copy selects an unavailable array layer");
        const auto& sourceMip = source.GetMipLayout(region.SourceMip);
        const auto& destinationMip = destination.GetMipLayout(region.DestinationMip);
        if (storageFormat(destination.GetFormat()) != storageFormat(source.GetFormat())) throw std::invalid_argument("Texture copy formats are incompatible");
        if (region.Width == 0 || region.Height == 0) throw std::invalid_argument("Texture copy requires a nonempty region");
        if (region.SourceX >= sourceMip.Width || region.SourceY >= sourceMip.Height || region.Width > sourceMip.Width - region.SourceX || region.Height > sourceMip.Height - region.SourceY) throw std::out_of_range("Texture copy source region exceeds its texture");
        if (region.DestinationX >= destinationMip.Width || region.DestinationY >= destinationMip.Height || region.Width > destinationMip.Width - region.DestinationX || region.Height > destinationMip.Height - region.DestinationY) throw std::out_of_range("Texture copy destination region exceeds its texture");
        const auto& sourceStorage = source.GetStorage();
        const auto& destinationStorage = destination.GetStorage();
        if (sourceStorage.Data() == destinationStorage.Data() && region.SourceMip == region.DestinationMip && region.SourceLayer == region.DestinationLayer && region.SourceX < region.DestinationX + region.Width && region.DestinationX < region.SourceX + region.Width && region.SourceY < region.DestinationY + region.Height && region.DestinationY < region.SourceY + region.Height) throw std::invalid_argument("Texture copy regions overlap");
    }

    std::vector<TextureCopyRegion> BuildTextureCopyRegions(const LinearTexture2D& destination, const LinearTexture2D& source, TextureCopyRegion firstRegion, std::uint32_t mipCount, std::uint32_t layerCount)
    {
        if (mipCount == 0 || firstRegion.SourceMip >= source.GetMipCount() || firstRegion.DestinationMip >= destination.GetMipCount() || mipCount > source.GetMipCount() - firstRegion.SourceMip || mipCount > destination.GetMipCount() - firstRegion.DestinationMip) throw std::out_of_range("Texture copy exceeds its mip chain");
        if (layerCount == 0 || firstRegion.SourceLayer >= source.GetArrayLayers() || firstRegion.DestinationLayer >= destination.GetArrayLayers() || layerCount > source.GetArrayLayers() - firstRegion.SourceLayer || layerCount > destination.GetArrayLayers() - firstRegion.DestinationLayer) throw std::out_of_range("Texture copy exceeds its array layers");
        const bool mipOverlap = firstRegion.SourceMip < firstRegion.DestinationMip + mipCount && firstRegion.DestinationMip < firstRegion.SourceMip + mipCount;
        const bool layerOverlap = firstRegion.SourceLayer < firstRegion.DestinationLayer + layerCount && firstRegion.DestinationLayer < firstRegion.SourceLayer + layerCount;
        if (source.GetStorage().Data() == destination.GetStorage().Data() && mipOverlap && layerOverlap && (firstRegion.SourceMip != firstRegion.DestinationMip || firstRegion.SourceLayer != firstRegion.DestinationLayer)) throw std::invalid_argument("Texture copy subresource ranges overlap");
        std::vector<TextureCopyRegion> regions;
        regions.reserve(static_cast<std::size_t>(mipCount) * layerCount);
        for (std::uint32_t mip = 0; mip < mipCount; ++mip)
        {
            for (std::uint32_t layer = 0; layer < layerCount; ++layer)
            {
                auto region = firstRegion;
                region.SourceLayer += layer;
                region.DestinationLayer += layer;
                ValidateTextureCopy(destination, source, region);
                regions.push_back(region);
            }
            firstRegion.SourceX /= 2;
            firstRegion.SourceY /= 2;
            firstRegion.DestinationX /= 2;
            firstRegion.DestinationY /= 2;
            firstRegion.Width = std::max(firstRegion.Width / 2, 1u);
            firstRegion.Height = std::max(firstRegion.Height / 2, 1u);
            ++firstRegion.SourceMip;
            ++firstRegion.DestinationMip;
        }
        return regions;
    }

    void WriteTextureCopy(CommandBuffer& commands, const LinearTexture2D& destination, const LinearTexture2D& source, const TextureCopyRegion& region)
    {
        ValidateTextureCopy(destination, source, region);
        const auto& sourceMip = source.GetMipLayout(region.SourceMip);
        const auto& destinationMip = destination.GetMipLayout(region.DestinationMip);
        const auto& sourceStorage = source.GetStorage();
        const auto& destinationStorage = destination.GetStorage();
        const auto pixelBytes = source.GetRowBytes() / source.GetWidth();
        const auto rowBytes = region.Width * pixelBytes;
        const GpuMemoryRange ranges[]{{sourceStorage.Data(), sourceStorage.Size()}, {destinationStorage.Data(), destinationStorage.Size()}};
        WriteMemoryAcquire(commands, ranges);
        for (std::uint32_t row = 0; row < region.Height; ++row)
        {
            const auto sourceOffset = region.SourceLayer * source.GetLayerPitch() + sourceMip.Offset + (region.SourceY + row) * sourceMip.RowPitch + region.SourceX * pixelBytes;
            const auto destinationOffset = region.DestinationLayer * destination.GetLayerPitch() + destinationMip.Offset + (region.DestinationY + row) * destinationMip.RowPitch + region.DestinationX * pixelBytes;
            WriteBufferCopy(commands, destinationStorage.GetRange(destinationOffset, rowBytes).data(), sourceStorage.GetRange(sourceOffset, rowBytes).data(), rowBytes);
        }
    }
}
