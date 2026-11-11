#include "AnyUnreal/RHI/Command/TextureCopy.h"
#include "AnyUnreal/RHI/Command/BufferCopy.h"
#include "AnyUnreal/RHI/Command/MemoryAcquire.h"
#include <stdexcept>

namespace AnyUnreal::RHI
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

    void WriteTextureCopy(CommandBuffer& commands, const LinearTexture3D& destination, const LinearTexture3D& source, const VolumeCopyRegion& region)
    {
        if (storageFormat(destination.GetFormat()) != storageFormat(source.GetFormat())) throw std::invalid_argument("Volume copy formats are incompatible");
        const std::array sourceSize{source.GetWidth(), source.GetHeight(), source.GetDepth()};
        const std::array destinationSize{destination.GetWidth(), destination.GetHeight(), destination.GetDepth()};
        bool overlap = source.GetStorage().Data() == destination.GetStorage().Data();
        for (std::size_t axis = 0; axis < 3; ++axis)
        {
            const auto size = region.Extent[axis];
            if (size == 0) throw std::invalid_argument("Volume copy requires a nonempty region");
            if (region.Source[axis] >= sourceSize[axis] || size > sourceSize[axis] - region.Source[axis]) throw std::out_of_range("Volume copy exceeds its source");
            if (region.Destination[axis] >= destinationSize[axis] || size > destinationSize[axis] - region.Destination[axis]) throw std::out_of_range("Volume copy exceeds its destination");
            overlap = overlap && region.Source[axis] < region.Destination[axis] + size && region.Destination[axis] < region.Source[axis] + size;
        }
        if (overlap) throw std::invalid_argument("Volume copy regions overlap");
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
                const auto from = (region.Source[2] + z) * source.GetSlicePitch() + (region.Source[1] + y) * source.GetRowPitch() + region.Source[0] * pixelBytes;
                const auto to = (region.Destination[2] + z) * destination.GetSlicePitch() + (region.Destination[1] + y) * destination.GetRowPitch() + region.Destination[0] * pixelBytes;
                WriteBufferCopy(commands, destinationStorage.GetRange(to, rowBytes).data(), sourceStorage.GetRange(from, rowBytes).data(), rowBytes);
            }
        }
    }

    void WriteTextureCopy(CommandBuffer& commands, const LinearTexture2D& destination, const LinearTexture2D& source, const TextureCopyRegion& region)
    {
        if (storageFormat(destination.GetFormat()) != storageFormat(source.GetFormat())) throw std::invalid_argument("Texture copy formats are incompatible");
        if (region.Width == 0 || region.Height == 0) throw std::invalid_argument("Texture copy requires a nonempty region");
        if (region.SourceX >= source.GetWidth() || region.SourceY >= source.GetHeight() || region.Width > source.GetWidth() - region.SourceX || region.Height > source.GetHeight() - region.SourceY) throw std::out_of_range("Texture copy source region exceeds its texture");
        if (region.DestinationX >= destination.GetWidth() || region.DestinationY >= destination.GetHeight() || region.Width > destination.GetWidth() - region.DestinationX || region.Height > destination.GetHeight() - region.DestinationY) throw std::out_of_range("Texture copy destination region exceeds its texture");
        const auto& sourceStorage = source.GetStorage();
        const auto& destinationStorage = destination.GetStorage();
        if (sourceStorage.Data() == destinationStorage.Data() && region.SourceX < region.DestinationX + region.Width && region.DestinationX < region.SourceX + region.Width && region.SourceY < region.DestinationY + region.Height && region.DestinationY < region.SourceY + region.Height) throw std::invalid_argument("Texture copy regions overlap");
        const auto pixelBytes = source.GetRowBytes() / source.GetWidth();
        const auto rowBytes = region.Width * pixelBytes;
        const GpuMemoryRange ranges[]{{sourceStorage.Data(), sourceStorage.Size()}, {destinationStorage.Data(), destinationStorage.Size()}};
        WriteMemoryAcquire(commands, ranges);
        for (std::uint32_t row = 0; row < region.Height; ++row)
        {
            const auto sourceOffset = source.GetMipLayout(0).Offset + (region.SourceY + row) * source.GetRowPitch() + region.SourceX * pixelBytes;
            const auto destinationOffset = destination.GetMipLayout(0).Offset + (region.DestinationY + row) * destination.GetRowPitch() + region.DestinationX * pixelBytes;
            WriteBufferCopy(commands, destinationStorage.GetRange(destinationOffset, rowBytes).data(), sourceStorage.GetRange(sourceOffset, rowBytes).data(), rowBytes);
        }
    }
}
