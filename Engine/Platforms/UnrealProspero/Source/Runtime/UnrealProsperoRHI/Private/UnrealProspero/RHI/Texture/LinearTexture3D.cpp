#include "UnrealProspero/RHI/Texture/LinearTexture3D.h"
#include "UnrealProspero/RHI/Device/DeviceLimits.h"
#include <cstring>
#include <limits>
#include <stdexcept>

namespace UnrealProspero::RHI
{
    namespace
    {
        std::vector<LinearMipLayout> calculateMipLayout(std::uint32_t width, std::uint32_t height, std::uint32_t depth, TextureFormat format, std::uint32_t mipCount)
        {
            const auto info = DescribeTextureFormat(format);
            return CalculateLinearMipLayout(width, height, depth, info.BlockBytes, mipCount, info.BlockWidth, info.BlockHeight);
        }
    }

    std::size_t LinearTexture3D::CalculateDataSize(std::uint32_t width, std::uint32_t height, std::uint32_t depth, TextureFormat format, std::uint32_t mipCount, std::uint32_t firstMip)
    {
        if (width == 0 || height == 0 || depth == 0 || width > DeviceLimits::Texture3D || height > DeviceLimits::Texture3D || depth > DeviceLimits::Texture3D) throw std::out_of_range("Volume texture dimensions exceed the hardware range");
        const auto levels = calculateMipLayout(width, height, depth, format, mipCount);
        if (firstMip >= levels.size()) throw std::out_of_range("Volume allocation selects an unavailable mip");
        const auto size = (levels[firstMip].Offset + levels[firstMip].Size) * depth;
        BufferAllocation::CalculateAllocationSize(size);
        return size;
    }

    LinearTexture3D::LinearTexture3D(std::uint32_t width, std::uint32_t height, std::uint32_t depth, TextureFormat format, std::uint32_t addressHigh, std::uint32_t mipCount) : width(width), height(height), depth(depth), format(format), mipLayouts(calculateMipLayout(width, height, depth, format, mipCount)), rowPitch(mipLayouts.front().RowPitch), slicePitch(mipLayouts.front().Offset + mipLayouts.front().Size), storage(CalculateDataSize(width, height, depth, format, mipCount), addressHigh, Kernel::DirectAllocation::PageSize, AllocationUsage::Texture) {}

    std::size_t LinearTexture3D::validateLayout(std::size_t size, std::size_t dataRowPitch, std::size_t dataSlicePitch, const LinearMipLayout& mip) const
    {
        if (dataRowPitch < mip.RowBytes || dataRowPitch > std::numeric_limits<std::size_t>::max() / mip.RowCount || dataSlicePitch < dataRowPitch * mip.RowCount) throw std::invalid_argument("Volume row or slice pitch is too small");
        const auto lastSliceBytes = dataRowPitch * (mip.RowCount - 1) + mip.RowBytes;
        if (size < lastSliceBytes || mip.Depth - 1 > (size - lastSliceBytes) / dataSlicePitch) throw std::invalid_argument("Volume data does not contain all slices");
        return dataSlicePitch * (mip.Depth - 1) + lastSliceBytes;
    }

    void LinearTexture3D::Upload(std::span<const std::byte> source, std::size_t sourceRowPitch, std::size_t sourceSlicePitch, std::uint32_t mipLevel)
    {
        const auto& mip = GetMipLayout(mipLevel);
        if (!source.data()) throw std::invalid_argument("Volume upload source is null");
        const auto bytes = validateLayout(source.size(), sourceRowPitch, sourceSlicePitch, mip);
        const auto address = reinterpret_cast<std::uintptr_t>(storage.Data());
        const auto input = reinterpret_cast<std::uintptr_t>(source.data());
        if ((input >= address && input - address < storage.Size()) || (address >= input && address - input < bytes)) throw std::invalid_argument("Volume upload overlaps its storage");
        for (std::size_t z = 0; z < mip.Depth; ++z)
        {
            for (std::size_t y = 0; y < mip.RowCount; ++y) storage.Upload(mip.Offset + z * slicePitch + y * mip.RowPitch, source.subspan(z * sourceSlicePitch + y * sourceRowPitch, mip.RowBytes));
        }
    }

    void LinearTexture3D::CopyTo(std::span<std::byte> destination, std::size_t destinationRowPitch, std::size_t destinationSlicePitch, std::uint32_t mipLevel) const
    {
        const auto& mip = GetMipLayout(mipLevel);
        if (!destination.data()) throw std::invalid_argument("Volume readback destination is null");
        const auto bytes = validateLayout(destination.size(), destinationRowPitch, destinationSlicePitch, mip);
        const auto address = reinterpret_cast<std::uintptr_t>(storage.Data());
        const auto target = reinterpret_cast<std::uintptr_t>(destination.data());
        if ((target >= address && target - address < storage.Size()) || (address >= target && address - target < bytes)) throw std::invalid_argument("Volume readback overlaps its storage");
        const auto* source = static_cast<const std::byte*>(storage.Data());
        for (std::size_t z = 0; z < mip.Depth; ++z)
        {
            for (std::size_t y = 0; y < mip.RowCount; ++y) std::memcpy(destination.data() + z * destinationSlicePitch + y * destinationRowPitch, source + mip.Offset + z * slicePitch + y * mip.RowPitch, mip.RowBytes);
        }
    }

    std::array<std::uint32_t, 8> LinearTexture3D::GetDescriptor() const
    {
        return CreateViewDescriptor(false);
    }

    std::array<std::uint32_t, 8> LinearTexture3D::CreateViewDescriptor(bool disableSrgb, std::uint32_t firstMip, std::uint32_t mipCount) const
    {
        if (firstMip >= mipLayouts.size()) throw std::out_of_range("Volume view selects an unavailable mip");
        if (mipCount == 0) mipCount = GetMipCount() - firstMip;
        if (mipCount > GetMipCount() - firstMip) throw std::out_of_range("Volume view exceeds its mip chain");
        auto result = LinearTexture2D::CreateDescriptor(width, height, format, storage.Data(), disableSrgb);
        result[3] = (result[3] & 0x0fffffffu) | (10u << 28);
        result[3] |= (firstMip << 12) | ((firstMip + mipCount - 1) << 16);
        result[4] = depth - 1;
        result[5] = (result[5] & ~0xf0u) | ((GetMipCount() - 1) << 4);
        return result;
    }

    const LinearMipLayout& LinearTexture3D::GetMipLayout(std::uint32_t mipLevel) const
    {
        if (mipLevel >= mipLayouts.size()) throw std::out_of_range("Volume mip does not exist");
        return mipLayouts[mipLevel];
    }

    std::uint32_t LinearTexture3D::GetMipCount() const { return static_cast<std::uint32_t>(mipLayouts.size()); }

    std::span<std::byte> LinearTexture3D::GetSliceRange(std::uint32_t mipLevel, std::uint32_t slice) const
    {
        const auto& mip = GetMipLayout(mipLevel);
        if (slice >= mip.Depth) throw std::out_of_range("Volume mip slice does not exist");
        return storage.GetRange(mip.Offset + slice * slicePitch, mip.Size);
    }

    const BufferAllocation& LinearTexture3D::GetStorage() const { return storage; }
    std::size_t LinearTexture3D::GetRowPitch() const { return rowPitch; }
    std::size_t LinearTexture3D::GetSlicePitch() const { return slicePitch; }
    std::uint32_t LinearTexture3D::GetWidth() const { return width; }
    std::uint32_t LinearTexture3D::GetHeight() const { return height; }
    std::uint32_t LinearTexture3D::GetDepth() const { return depth; }
    TextureFormat LinearTexture3D::GetFormat() const { return format; }
}
