#include "AnyUnreal/RHI/Memory/VolumeUpload.h"
#include "AnyUnreal/RHI/Command/TextureCopy.h"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    VolumeUpload::VolumeUpload(std::uint32_t width, std::uint32_t height, std::uint32_t depth, TextureFormat format, std::span<const std::byte> source, std::size_t rowPitch, std::size_t slicePitch, std::uint32_t addressHigh) : storage(width, height, depth, format, addressHigh), barrier(addressHigh)
    {
        storage.Upload(source, rowPitch, slicePitch);
    }

    void VolumeUpload::Record(CommandBuffer& commands, const LinearTexture3D& destination, std::uint32_t x, std::uint32_t y, std::uint32_t z)
    {
        if (recorded) throw std::runtime_error("Volume upload has already been recorded");
        if (destination.GetFormat() != storage.GetFormat()) throw std::invalid_argument("Volume upload formats do not match");
        if (x >= destination.GetWidth() || y >= destination.GetHeight() || z >= destination.GetDepth() || storage.GetWidth() > destination.GetWidth() - x || storage.GetHeight() > destination.GetHeight() - y || storage.GetDepth() > destination.GetDepth() - z) throw std::out_of_range("Volume upload exceeds the destination");
        const auto& sourceAllocation = storage.GetStorage();
        const auto& destinationAllocation = destination.GetStorage();
        const GpuMemoryRange ranges[]{{sourceAllocation.Data(), sourceAllocation.Size()}, {destinationAllocation.Data(), destinationAllocation.Size()}};
        recorded = true;
        barrier.Release(commands);
        barrier.Acquire(commands, ranges);
        WriteTextureCopy(commands, destination, storage, {{0, 0, 0}, {x, y, z}, {storage.GetWidth(), storage.GetHeight(), storage.GetDepth()}});
    }
}
