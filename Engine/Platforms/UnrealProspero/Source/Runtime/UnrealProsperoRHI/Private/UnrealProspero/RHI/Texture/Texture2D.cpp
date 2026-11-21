#include "UnrealProspero/RHI/Texture/Texture2D.h"
#include "UnrealProspero/RHI/Device/DeviceLimits.h"
#include <stdexcept>
#include <algorithm>
#include <bit>
#include <cstring>

namespace UnrealProspero::RHI
{
    namespace
    {
        std::uint32_t layerCount(std::uint32_t value)
        {
            if (value == 0 || value > 8192) throw std::out_of_range("Texture array layer count exceeds the descriptor range");
            return value;
        }

        std::uint32_t dimension(std::uint32_t value)
        {
            if (value == 0 || value > DeviceLimits::Texture2D) throw std::out_of_range("Texture dimension exceeds the hardware range");
            return value;
        }
    }

    std::vector<Texture2D::MipLayout> Texture2D::CalculateMipLayout(std::uint32_t width, std::uint32_t height, TextureFormat format, std::uint32_t mipCount, Texture2DLayout layout)
    {
        const auto info = DescribeTextureFormat(format);
        if (layout == Texture2DLayout::RenderTarget64K)
        {
            dimension(width);
            dimension(height);
            if (mipCount != 1 || info.BlockBytes != 4 || info.BlockWidth != 1 || info.BlockHeight != 1) throw std::invalid_argument("Tiled display storage requires one mip of 32-bit pixels");
            const auto size = static_cast<std::size_t>((width + 127) / 128) * ((height + 127) / 128) * 65536;
            return {{width, height, 1, static_cast<std::size_t>(width) * 4, CalculateRowPitch(width, format, layout), 0, size, height}};
        }
        if (layout != Texture2DLayout::Linear) throw std::invalid_argument("Unknown 2D texture layout");
        return CalculateLinearMipLayout(width, height, 1, info.BlockBytes, mipCount, info.BlockWidth, info.BlockHeight);
    }

    std::size_t Texture2D::CalculateRowBytes(std::uint32_t width, TextureFormat format)
    {
        const auto info = DescribeTextureFormat(format);
        return ((dimension(width) + info.BlockWidth - 1) / info.BlockWidth) * info.BlockBytes;
    }

    std::size_t Texture2D::CalculateRowPitch(std::uint32_t width, TextureFormat format, Texture2DLayout layout)
    {
        if (layout == Texture2DLayout::RenderTarget64K)
        {
            const auto info = DescribeTextureFormat(format);
            if (info.BlockBytes != 4 || info.BlockWidth != 1 || info.BlockHeight != 1) throw std::invalid_argument("Tiled display storage requires 32-bit pixels");
            return static_cast<std::size_t>((dimension(width) + 127) / 128) * 512;
        }
        if (layout != Texture2DLayout::Linear) throw std::invalid_argument("Unknown 2D texture layout");
        return (CalculateRowBytes(width, format) + 255) & ~std::size_t{255};
    }

    std::size_t Texture2D::CalculateDataSize(std::uint32_t width, std::uint32_t height, TextureFormat format, std::uint32_t mipCount, std::uint32_t firstMip, Texture2DLayout layout)
    {
        const auto levels = CalculateMipLayout(width, height, format, mipCount, layout);
        if (firstMip >= levels.size()) throw std::out_of_range("Texture size selects an unavailable mip");
        return levels[firstMip].Offset + levels[firstMip].Size;
    }

    Texture2D::Texture2D(std::uint32_t width, std::uint32_t height, TextureFormat format, std::uint32_t addressHigh, std::size_t alignment, std::uint32_t mipCount, AllocationUsage usage, std::uint32_t arrayLayers, Texture2DLayout layout) : width(dimension(width)), height(dimension(height)), format(format), layout(layout), rowBytes(CalculateRowBytes(width, format)), rowPitch(CalculateRowPitch(width, format, layout)), mipLayouts(CalculateMipLayout(width, height, format, mipCount, layout)), arrayLayers(layerCount(arrayLayers)), layerPitch(mipLayouts.front().Offset + mipLayouts.front().Size), storage(layerPitch * this->arrayLayers, addressHigh, alignment, usage)
    {
        if (usage != AllocationUsage::Texture && usage != AllocationUsage::StreamingTexture) throw std::invalid_argument("Texture requires texture allocation usage");
        if (layout == Texture2DLayout::RenderTarget64K && (alignment < 65536 || arrayLayers != 1)) throw std::invalid_argument("Tiled display storage requires a single 64 KiB aligned layer");
        descriptor = arrayLayers == 1 ? CreateDescriptor(width, height, format, storage.Data(), false, mipCount, layout) : CreateArrayViewDescriptor(false, 0, arrayLayers);
    }

    std::array<std::uint32_t, 8> Texture2D::CreateDescriptor(std::uint32_t width, std::uint32_t height, TextureFormat format, const void* data, bool disableSrgb, std::uint32_t mipCount, Texture2DLayout layout)
    {
        dimension(width);
        dimension(height);
        if (mipCount == 0 || mipCount > static_cast<std::uint32_t>(std::bit_width(std::max(width, height)))) throw std::invalid_argument("Texture descriptor mip count exceeds its dimensions");
        const auto address = reinterpret_cast<std::uintptr_t>(data);
        if (address == 0 || address % 256 != 0 || address >= (std::uint64_t{1} << 48)) throw std::runtime_error("Texture storage address cannot be encoded");
        const auto info = DescribeTextureFormat(format, disableSrgb);
        if (layout == Texture2DLayout::RenderTarget64K)
        {
            static_cast<void>(CalculateMipLayout(width, height, format, mipCount, layout));
            if (address % 65536 != 0) throw std::invalid_argument("Tiled texture requires 64 KiB alignment");
        }
        else if (layout != Texture2DLayout::Linear) throw std::invalid_argument("Unknown 2D texture layout");
        return {
            static_cast<std::uint32_t>(address >> 8),
            static_cast<std::uint32_t>(address >> 40) | (info.Code << 20) | (((width - 1) & 3) << 30),
            ((width - 1) >> 2) | ((height - 1) << 14) | (1u << 31),
            info.Swizzle | ((mipCount - 1) << 16) | ((layout == Texture2DLayout::RenderTarget64K ? 27u : 0u) << 20) | (9u << 28),
            0, (mipCount - 1) << 4, 0, 0
        };
    }

    void Texture2D::Upload(std::span<const std::byte> source, std::size_t sourcePitch, std::uint32_t mipLevel, std::uint32_t arrayLayer)
    {
        const auto& mip = GetMipLayout(mipLevel);
        GetMipRange(mipLevel, arrayLayer);
        if (!source.data() || sourcePitch < mip.RowBytes || source.size() < mip.RowBytes || (mip.RowCount - 1) > (source.size() - mip.RowBytes) / sourcePitch) throw std::invalid_argument("Texture upload does not contain all source rows");
        if (layout == Texture2DLayout::RenderTarget64K)
        {
            for (std::uint32_t y = 0; y < height; ++y)
                for (std::uint32_t x = 0; x < width; ++x)
                    storage.Upload(tiledTexelOffset(x, y), source.subspan(y * sourcePitch + x * 4, 4));
        }
        else for (std::size_t row = 0; row < mip.RowCount; ++row) storage.Upload(arrayLayer * layerPitch + mip.Offset + row * mip.RowPitch, source.subspan(row * sourcePitch, mip.RowBytes));
    }

    void Texture2D::CopyRowsTo(std::span<std::byte> destination, std::size_t destinationPitch, std::uint32_t mipLevel, std::uint32_t arrayLayer) const
    {
        const auto& mip = GetMipLayout(mipLevel);
        const auto range = GetMipRange(mipLevel, arrayLayer);
        if (!destination.data() || destinationPitch < mip.RowBytes || destination.size() < mip.RowBytes || (mip.RowCount - 1) > (destination.size() - mip.RowBytes) / destinationPitch) throw std::invalid_argument("Texture readback does not contain all destination rows");
        const auto sourceAddress = reinterpret_cast<std::uintptr_t>(storage.Data());
        const auto destinationAddress = reinterpret_cast<std::uintptr_t>(destination.data());
        if ((destinationAddress >= sourceAddress && destinationAddress - sourceAddress < storage.Size()) || (sourceAddress >= destinationAddress && sourceAddress - destinationAddress < destination.size())) throw std::invalid_argument("Texture read destination overlaps its storage");
        const auto* source = range.data();
        if (layout == Texture2DLayout::RenderTarget64K)
        {
            for (std::uint32_t y = 0; y < height; ++y)
                for (std::uint32_t x = 0; x < width; ++x)
                    std::memcpy(destination.data() + y * destinationPitch + x * 4, source + tiledTexelOffset(x, y), 4);
        }
        else for (std::size_t row = 0; row < mip.RowCount; ++row) std::memcpy(destination.data() + row * destinationPitch, source + row * mip.RowPitch, mip.RowBytes);
    }

    const Texture2D::MipLayout& Texture2D::GetMipLayout(std::uint32_t mipLevel) const
    {
        if (mipLevel >= mipLayouts.size()) throw std::out_of_range("Texture mip level exceeds its allocation");
        return mipLayouts[mipLevel];
    }

    std::uint32_t Texture2D::GetMipCount() const { return static_cast<std::uint32_t>(mipLayouts.size()); }

    std::span<std::byte> Texture2D::GetMipRange(std::uint32_t mipLevel, std::uint32_t arrayLayer) const
    {
        const auto& mip = GetMipLayout(mipLevel);
        if (arrayLayer >= arrayLayers) throw std::out_of_range("Texture array layer exceeds its allocation");
        return storage.GetRange(arrayLayer * layerPitch + mip.Offset, mip.Size);
    }

    const std::array<std::uint32_t, 8>& Texture2D::GetDescriptor() const { return descriptor; }
    std::array<std::uint32_t, 8> Texture2D::CreateViewDescriptor(bool disableSrgb, std::uint32_t firstMip, std::uint32_t mipCount, std::uint32_t arrayLayer) const
    {
        if (firstMip >= GetMipCount()) throw std::out_of_range("Texture view starts past the mip chain");
        if (mipCount == 0) mipCount = GetMipCount() - firstMip;
        if (mipCount > GetMipCount() - firstMip) throw std::out_of_range("Texture view exceeds the mip chain");
        if (arrayLayer >= arrayLayers) throw std::out_of_range("Texture view layer exceeds its allocation");
        auto result = CreateDescriptor(width, height, format, storage.GetRange(arrayLayer * layerPitch, layerPitch).data(), disableSrgb, GetMipCount(), layout);
        result[3] = (result[3] & ~0xff000u) | (firstMip << 12) | ((firstMip + mipCount - 1) << 16);
        return result;
    }

    std::array<std::uint32_t, 8> Texture2D::CreateArrayViewDescriptor(bool disableSrgb, std::uint32_t firstLayer, std::uint32_t count, std::uint32_t firstMip, std::uint32_t mipCount) const
    {
        if (firstLayer >= arrayLayers || count == 0 || count > arrayLayers - firstLayer) throw std::out_of_range("Texture array view exceeds its layers");
        auto result = CreateViewDescriptor(disableSrgb, firstMip, mipCount);
        result[3] = (result[3] & 0x0fffffffu) | (13u << 28);
        result[4] = (firstLayer + count - 1) | (firstLayer << 16);
        return result;
    }

    std::array<std::uint32_t, 8> Texture2D::CreateCubeViewDescriptor(bool disableSrgb, std::uint32_t firstCube, std::uint32_t cubeCount, std::uint32_t firstMip, std::uint32_t mipCount) const
    {
        if (width != height || arrayLayers % 6 != 0) throw std::invalid_argument("Cube view requires square faces in groups of six");
        const auto cubes = arrayLayers / 6;
        if (firstCube >= cubes || cubeCount == 0 || cubeCount > cubes - firstCube) throw std::out_of_range("Cube view exceeds its face allocation");
        auto result = CreateArrayViewDescriptor(disableSrgb, firstCube * 6, cubeCount * 6, firstMip, mipCount);
        result[3] = (result[3] & 0x0fffffffu) | (11u << 28);
        return result;
    }

    std::uint32_t Texture2D::GetArrayLayers() const { return arrayLayers; }
    std::size_t Texture2D::GetLayerPitch() const { return layerPitch; }

    const BufferAllocation& Texture2D::GetStorage() const { return storage; }
    std::size_t Texture2D::GetRowPitch() const { return rowPitch; }
    std::size_t Texture2D::GetRowBytes() const { return rowBytes; }
    std::uint32_t Texture2D::GetWidth() const { return width; }
    std::uint32_t Texture2D::GetHeight() const { return height; }
    TextureFormat Texture2D::GetFormat() const { return format; }
    Texture2DLayout Texture2D::GetLayout() const { return layout; }

    std::size_t Texture2D::tiledTexelOffset(std::uint32_t x, std::uint32_t y) const
    {
        constexpr std::array<std::uint32_t, 16> equation{0, 0, 1, 2, 0x1000, 0x2000, 0x4000, 4, 0x8008008, 0x4010010, 0x2020040, 0x1040020, 0x8000, 0x10, 0x40000, 0x40};
        const auto coordinates = (x & 127u) | ((y & 127u) << 12);
        std::size_t offset = (static_cast<std::size_t>(y / 128) * ((width + 127) / 128) + x / 128) * 65536;
        for (std::size_t bit = 2; bit < equation.size(); ++bit) offset |= static_cast<std::size_t>(std::popcount(coordinates & equation[bit]) & 1) << bit;
        return offset;
    }
}
