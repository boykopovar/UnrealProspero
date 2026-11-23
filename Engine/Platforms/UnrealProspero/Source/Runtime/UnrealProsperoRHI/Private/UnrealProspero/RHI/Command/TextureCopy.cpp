#include "UnrealProspero/RHI/Command/TextureCopy.h"
#include "UnrealProspero/RHI/Command/BufferCopy.h"
#include "UnrealProspero/RHI/Command/MemoryAcquire.h"
#include <stdexcept>
#include <algorithm>

namespace UnrealProspero::RHI
{
    namespace
    {
        bool compatible(TextureFormat left, TextureFormat right)
        {
            const auto a = DescribeTextureFormat(left, true);
            const auto b = DescribeTextureFormat(right, true);
            return a.Code == b.Code && a.Swizzle == b.Swizzle;
        }

        void validateBlockAxis(std::uint32_t source, std::uint32_t destination, std::uint32_t extent, std::uint32_t sourceSize, std::uint32_t destinationSize, std::uint32_t block)
        {
            if (source % block != 0 || destination % block != 0) throw std::invalid_argument("Texture copy origin is not block aligned");
            if (extent % block != 0 && (extent != sourceSize - source || extent != destinationSize - destination)) throw std::invalid_argument("Texture copy splits a compressed block");
        }

    }

    void ValidateTextureCopy(const LinearTexture3D& destination, const LinearTexture3D& source, const VolumeCopyRegion& region)
    {
        if (!compatible(destination.GetFormat(), source.GetFormat())) throw std::invalid_argument("Volume copy formats are incompatible");
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
        const auto info = DescribeTextureFormat(source.GetFormat());
        validateBlockAxis(region.Source[0], region.Destination[0], region.Extent[0], sourceMip.Width, destinationMip.Width, info.BlockWidth);
        validateBlockAxis(region.Source[1], region.Destination[1], region.Extent[1], sourceMip.Height, destinationMip.Height, info.BlockHeight);
        if (overlap) throw std::invalid_argument("Volume copy regions overlap");
    }

    void WriteTextureCopy(CommandBuffer& commands, const LinearTexture3D& destination, const LinearTexture3D& source, const VolumeCopyRegion& region)
    {
        ValidateTextureCopy(destination, source, region);
        const auto& sourceMip = source.GetMipLayout(region.SourceMip);
        const auto& destinationMip = destination.GetMipLayout(region.DestinationMip);
        const auto info = DescribeTextureFormat(source.GetFormat());
        const auto rowBytes = Texture2D::CalculateRowBytes(region.Extent[0], source.GetFormat());
        const auto rows = (region.Extent[1] + info.BlockHeight - 1) / info.BlockHeight;
        const auto& sourceStorage = source.GetStorage();
        const auto& destinationStorage = destination.GetStorage();
        const GpuMemoryRange ranges[]{{sourceStorage.Data(), sourceStorage.Size()}, {destinationStorage.Data(), destinationStorage.Size()}};
        WriteMemoryAcquire(commands, ranges);
        for (std::uint32_t z = 0; z < region.Extent[2]; ++z)
        {
            for (std::uint32_t y = 0; y < rows; ++y)
            {
                const auto from = sourceMip.Offset + (region.Source[2] + z) * source.GetSlicePitch() + (region.Source[1] / info.BlockHeight + y) * sourceMip.RowPitch + region.Source[0] / info.BlockWidth * info.BlockBytes;
                const auto to = destinationMip.Offset + (region.Destination[2] + z) * destination.GetSlicePitch() + (region.Destination[1] / info.BlockHeight + y) * destinationMip.RowPitch + region.Destination[0] / info.BlockWidth * info.BlockBytes;
                WriteBufferCopy(commands, destinationStorage.GetRange(to, rowBytes).data(), sourceStorage.GetRange(from, rowBytes).data(), rowBytes);
            }
        }
    }

    void ValidateTextureCopy(const Texture2D& destination, const Texture2D& source, const TextureCopyRegion& region)
    {
        if (destination.GetLayout() != Texture2DLayout::Linear || source.GetLayout() != Texture2DLayout::Linear) throw std::runtime_error("GPU copies of tiled color textures are not implemented");
        if (region.SourceLayer >= source.GetArrayLayers() || region.DestinationLayer >= destination.GetArrayLayers()) throw std::out_of_range("Texture copy selects an unavailable array layer");
        const auto& sourceMip = source.GetMipLayout(region.SourceMip);
        const auto& destinationMip = destination.GetMipLayout(region.DestinationMip);
        if (!compatible(destination.GetFormat(), source.GetFormat())) throw std::invalid_argument("Texture copy formats are incompatible");
        if (region.Width == 0 || region.Height == 0) throw std::invalid_argument("Texture copy requires a nonempty region");
        if (region.SourceX >= sourceMip.Width || region.SourceY >= sourceMip.Height || region.Width > sourceMip.Width - region.SourceX || region.Height > sourceMip.Height - region.SourceY) throw std::out_of_range("Texture copy source region exceeds its texture");
        if (region.DestinationX >= destinationMip.Width || region.DestinationY >= destinationMip.Height || region.Width > destinationMip.Width - region.DestinationX || region.Height > destinationMip.Height - region.DestinationY) throw std::out_of_range("Texture copy destination region exceeds its texture");
        const auto info = DescribeTextureFormat(source.GetFormat());
        validateBlockAxis(region.SourceX, region.DestinationX, region.Width, sourceMip.Width, destinationMip.Width, info.BlockWidth);
        validateBlockAxis(region.SourceY, region.DestinationY, region.Height, sourceMip.Height, destinationMip.Height, info.BlockHeight);
        const auto& sourceStorage = source.GetStorage();
        const auto& destinationStorage = destination.GetStorage();
        if (sourceStorage.Data() == destinationStorage.Data() && region.SourceMip == region.DestinationMip && region.SourceLayer == region.DestinationLayer && region.SourceX < region.DestinationX + region.Width && region.DestinationX < region.SourceX + region.Width && region.SourceY < region.DestinationY + region.Height && region.DestinationY < region.SourceY + region.Height) throw std::invalid_argument("Texture copy regions overlap");
    }

    std::vector<TextureCopyRegion> BuildTextureCopyRegions(const Texture2D& destination, const Texture2D& source, TextureCopyRegion firstRegion, std::uint32_t mipCount, std::uint32_t layerCount)
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

    void WriteTextureCopy(CommandBuffer& commands, const Texture2D& destination, const Texture2D& source, const TextureCopyRegion& region)
    {
        ValidateTextureCopy(destination, source, region);
        const auto& sourceMip = source.GetMipLayout(region.SourceMip);
        const auto& destinationMip = destination.GetMipLayout(region.DestinationMip);
        const auto& sourceStorage = source.GetStorage();
        const auto& destinationStorage = destination.GetStorage();
        const auto info = DescribeTextureFormat(source.GetFormat());
        const auto rowBytes = Texture2D::CalculateRowBytes(region.Width, source.GetFormat());
        const auto rows = (region.Height + info.BlockHeight - 1) / info.BlockHeight;
        const GpuMemoryRange ranges[]{{sourceStorage.Data(), sourceStorage.Size()}, {destinationStorage.Data(), destinationStorage.Size()}};
        WriteMemoryAcquire(commands, ranges);
        for (std::uint32_t row = 0; row < rows; ++row)
        {
            const auto sourceOffset = region.SourceLayer * source.GetLayerPitch() + sourceMip.Offset + (region.SourceY / info.BlockHeight + row) * sourceMip.RowPitch + region.SourceX / info.BlockWidth * info.BlockBytes;
            const auto destinationOffset = region.DestinationLayer * destination.GetLayerPitch() + destinationMip.Offset + (region.DestinationY / info.BlockHeight + row) * destinationMip.RowPitch + region.DestinationX / info.BlockWidth * info.BlockBytes;
            WriteBufferCopy(commands, destinationStorage.GetRange(destinationOffset, rowBytes).data(), sourceStorage.GetRange(sourceOffset, rowBytes).data(), rowBytes);
        }
    }
}
