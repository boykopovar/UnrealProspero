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
            const auto sourceOffset = (region.SourceY + row) * source.GetRowPitch() + region.SourceX * pixelBytes;
            const auto destinationOffset = (region.DestinationY + row) * destination.GetRowPitch() + region.DestinationX * pixelBytes;
            WriteBufferCopy(commands, destinationStorage.GetRange(destinationOffset, rowBytes).data(), sourceStorage.GetRange(sourceOffset, rowBytes).data(), rowBytes);
        }
    }
}
