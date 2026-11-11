#include "AnyUnreal/RHI/Texture/LinearTexture3D.h"
#include "AnyUnreal/RHI/Device/DeviceLimits.h"
#include <cstring>
#include <limits>
#include <stdexcept>

namespace AnyUnreal::RHI
{
    std::size_t LinearTexture3D::CalculateDataSize(std::uint32_t width, std::uint32_t height, std::uint32_t depth, TextureFormat format)
    {
        if (width == 0 || height == 0 || depth == 0 || width > DeviceLimits::Texture3D || height > DeviceLimits::Texture3D || depth > DeviceLimits::Texture3D) throw std::out_of_range("Volume texture dimensions exceed the hardware range");
        const auto size = LinearTexture2D::CalculateRowPitch(width, format) * height * depth;
        BufferAllocation::CalculateAllocationSize(size);
        return size;
    }

    LinearTexture3D::LinearTexture3D(std::uint32_t width, std::uint32_t height, std::uint32_t depth, TextureFormat format, std::uint32_t addressHigh) : width(width), height(height), depth(depth), format(format), rowBytes(LinearTexture2D::CalculateRowBytes(width, format)), rowPitch(LinearTexture2D::CalculateRowPitch(width, format)), slicePitch(rowPitch * height), storage(CalculateDataSize(width, height, depth, format), addressHigh, Kernel::DirectAllocation::PageSize, AllocationUsage::Texture) {}

    std::size_t LinearTexture3D::validateLayout(std::size_t size, std::size_t rowPitch, std::size_t slicePitch) const
    {
        if (rowPitch < rowBytes || rowPitch > std::numeric_limits<std::size_t>::max() / height || slicePitch < rowPitch * height) throw std::invalid_argument("Volume row or slice pitch is too small");
        const auto lastSliceBytes = rowPitch * (height - 1) + rowBytes;
        if (size < lastSliceBytes || depth - 1 > (size - lastSliceBytes) / slicePitch) throw std::invalid_argument("Volume data does not contain all slices");
        return slicePitch * (depth - 1) + lastSliceBytes;
    }

    void LinearTexture3D::Upload(std::span<const std::byte> source, std::size_t sourceRowPitch, std::size_t sourceSlicePitch)
    {
        if (!source.data()) throw std::invalid_argument("Volume upload source is null");
        validateLayout(source.size(), sourceRowPitch, sourceSlicePitch);
        for (std::size_t z = 0; z < depth; ++z)
        {
            for (std::size_t y = 0; y < height; ++y) storage.Upload(z * slicePitch + y * rowPitch, source.subspan(z * sourceSlicePitch + y * sourceRowPitch, rowBytes));
        }
    }

    void LinearTexture3D::CopyTo(std::span<std::byte> destination, std::size_t destinationRowPitch, std::size_t destinationSlicePitch) const
    {
        if (!destination.data()) throw std::invalid_argument("Volume readback destination is null");
        const auto bytes = validateLayout(destination.size(), destinationRowPitch, destinationSlicePitch);
        const auto address = reinterpret_cast<std::uintptr_t>(storage.Data());
        const auto target = reinterpret_cast<std::uintptr_t>(destination.data());
        if ((target >= address && target - address < storage.Size()) || (address >= target && address - target < bytes)) throw std::invalid_argument("Volume readback overlaps its storage");
        const auto* source = static_cast<const std::byte*>(storage.Data());
        for (std::size_t z = 0; z < depth; ++z)
        {
            for (std::size_t y = 0; y < height; ++y) std::memcpy(destination.data() + z * destinationSlicePitch + y * destinationRowPitch, source + z * slicePitch + y * rowPitch, rowBytes);
        }
    }

    std::array<std::uint32_t, 8> LinearTexture3D::GetDescriptor() const
    {
        return CreateViewDescriptor(false);
    }

    std::array<std::uint32_t, 8> LinearTexture3D::CreateViewDescriptor(bool disableSrgb) const
    {
        auto result = LinearTexture2D::CreateDescriptor(width, height, format, storage.Data(), disableSrgb);
        result[3] = (result[3] & 0x0fffffffu) | (10u << 28);
        result[4] = depth - 1;
        return result;
    }

    const BufferAllocation& LinearTexture3D::GetStorage() const { return storage; }
    std::size_t LinearTexture3D::GetRowPitch() const { return rowPitch; }
    std::size_t LinearTexture3D::GetSlicePitch() const { return slicePitch; }
    std::uint32_t LinearTexture3D::GetWidth() const { return width; }
    std::uint32_t LinearTexture3D::GetHeight() const { return height; }
    std::uint32_t LinearTexture3D::GetDepth() const { return depth; }
    TextureFormat LinearTexture3D::GetFormat() const { return format; }
}
