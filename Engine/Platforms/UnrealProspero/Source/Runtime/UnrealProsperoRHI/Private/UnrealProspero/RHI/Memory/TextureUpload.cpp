#include "UnrealProspero/RHI/Memory/TextureUpload.h"
#include <stdexcept>

namespace UnrealProspero::RHI
{
    TextureUpload::TextureUpload(std::uint32_t width, std::uint32_t height, TextureFormat format, std::span<const std::byte> source, std::size_t sourcePitch, std::uint32_t addressHigh) : storage(width, height, format, addressHigh), barrier(addressHigh)
    {
        storage.Upload(source, sourcePitch);
    }

    void TextureUpload::Record(CommandBuffer& commands, const LinearTexture2D& destination, std::uint32_t x, std::uint32_t y, std::uint32_t mipLevel, std::uint32_t arrayLayer)
    {
        if (recorded) throw std::runtime_error("Texture upload has already been recorded");
        if (destination.GetFormat() != storage.GetFormat()) throw std::invalid_argument("Texture upload formats do not match");
        const auto& mip = destination.GetMipLayout(mipLevel);
        if (x >= mip.Width || y >= mip.Height || storage.GetWidth() > mip.Width - x || storage.GetHeight() > mip.Height - y) throw std::out_of_range("Texture upload exceeds the destination");
        if (arrayLayer >= destination.GetArrayLayers()) throw std::out_of_range("Texture upload selects an unavailable array layer");
        const TextureCopyRegion region{0, 0, x, y, storage.GetWidth(), storage.GetHeight(), 0, mipLevel, 0, arrayLayer};
        ValidateTextureCopy(destination, storage, region);
        recorded = true;
        const auto& allocation = destination.GetStorage();
        const GpuMemoryRange ranges[]{{allocation.Data(), allocation.Size()}};
        barrier.Release(commands);
        barrier.Acquire(commands, ranges);
        WriteTextureCopy(commands, destination, storage, region);
    }
}
