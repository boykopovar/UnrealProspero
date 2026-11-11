#include "UnrealProspero/RHI/Texture/LinearTexture2D.h"
#include "UnrealProspero/RHI/Device/DeviceLimits.h"
#include <stdexcept>
#include <algorithm>
#include <bit>
#include <cstring>

namespace UnrealProspero::RHI
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
            case TextureFormat::R32Uint: return {20, 4, 4 | (1 << 9)};
            case TextureFormat::R32Sint: return {21, 4, 4 | (1 << 9)};
            case TextureFormat::R32Float: return {22, 4, 4 | (1 << 9)};
            case TextureFormat::Rgba16Float: return {71, 8, 0xfac};
            case TextureFormat::Rgba32Float: return {77, 16, 0xfac};
            default: throw std::invalid_argument("Unknown linear texture format");
            }
        }

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

    std::vector<LinearTexture2D::MipLayout> LinearTexture2D::CalculateMipLayout(std::uint32_t width, std::uint32_t height, TextureFormat format, std::uint32_t mipCount)
    {
        return CalculateLinearMipLayout(width, height, 1, describe(format).Bytes, mipCount);
    }

    std::size_t LinearTexture2D::CalculateRowBytes(std::uint32_t width, TextureFormat format)
    {
        return dimension(width) * describe(format).Bytes;
    }

    std::size_t LinearTexture2D::CalculateRowPitch(std::uint32_t width, TextureFormat format)
    {
        return (CalculateRowBytes(width, format) + 255) & ~std::size_t{255};
    }

    std::size_t LinearTexture2D::CalculateDataSize(std::uint32_t width, std::uint32_t height, TextureFormat format, std::uint32_t mipCount, std::uint32_t firstMip)
    {
        const auto levels = CalculateMipLayout(width, height, format, mipCount);
        if (firstMip >= levels.size()) throw std::out_of_range("Texture size selects an unavailable mip");
        return levels[firstMip].Offset + levels[firstMip].Size;
    }

    LinearTexture2D::LinearTexture2D(std::uint32_t width, std::uint32_t height, TextureFormat format, std::uint32_t addressHigh, std::size_t alignment, std::uint32_t mipCount, AllocationUsage usage, std::uint32_t arrayLayers) : width(dimension(width)), height(dimension(height)), format(format), rowBytes(CalculateRowBytes(width, format)), rowPitch(CalculateRowPitch(width, format)), mipLayouts(CalculateMipLayout(width, height, format, mipCount)), arrayLayers(layerCount(arrayLayers)), layerPitch(mipLayouts.front().Offset + mipLayouts.front().Size), storage(layerPitch * this->arrayLayers, addressHigh, alignment, usage)
    {
        if (usage != AllocationUsage::Texture && usage != AllocationUsage::StreamingTexture) throw std::invalid_argument("Texture requires texture allocation usage");
        descriptor = arrayLayers == 1 ? CreateDescriptor(width, height, format, storage.Data(), false, mipCount) : CreateArrayViewDescriptor(false, 0, arrayLayers);
    }

    std::array<std::uint32_t, 8> LinearTexture2D::CreateDescriptor(std::uint32_t width, std::uint32_t height, TextureFormat format, const void* data, bool disableSrgb, std::uint32_t mipCount)
    {
        dimension(width);
        dimension(height);
        if (mipCount == 0 || mipCount > static_cast<std::uint32_t>(std::bit_width(std::max(width, height)))) throw std::invalid_argument("Texture descriptor mip count exceeds its dimensions");
        const auto address = reinterpret_cast<std::uintptr_t>(data);
        if (address == 0 || address % 256 != 0 || address >= (std::uint64_t{1} << 48)) throw std::runtime_error("Texture storage address cannot be encoded");
        if (disableSrgb && format == TextureFormat::Rgba8Srgb) format = TextureFormat::Rgba8Unorm;
        if (disableSrgb && format == TextureFormat::Bgra8Srgb) format = TextureFormat::Bgra8Unorm;
        const auto info = describe(format);
        return {
            static_cast<std::uint32_t>(address >> 8),
            static_cast<std::uint32_t>(address >> 40) | (info.Code << 20) | (((width - 1) & 3) << 30),
            ((width - 1) >> 2) | ((height - 1) << 14),
            info.Swizzle | ((mipCount - 1) << 16) | (9u << 28),
            0, (mipCount - 1) << 4, 0, 0
        };
    }

    void LinearTexture2D::Upload(std::span<const std::byte> source, std::size_t sourcePitch, std::uint32_t mipLevel, std::uint32_t arrayLayer)
    {
        const auto& mip = GetMipLayout(mipLevel);
        GetMipRange(mipLevel, arrayLayer);
        if (!source.data() || sourcePitch < mip.RowBytes || source.size() < mip.RowBytes || (mip.Height - 1) > (source.size() - mip.RowBytes) / sourcePitch) throw std::invalid_argument("Texture upload does not contain all source rows");
        for (std::size_t row = 0; row < mip.Height; ++row) storage.Upload(arrayLayer * layerPitch + mip.Offset + row * mip.RowPitch, source.subspan(row * sourcePitch, mip.RowBytes));
    }

    void LinearTexture2D::CopyRowsTo(std::span<std::byte> destination, std::size_t destinationPitch, std::uint32_t mipLevel, std::uint32_t arrayLayer) const
    {
        const auto& mip = GetMipLayout(mipLevel);
        const auto range = GetMipRange(mipLevel, arrayLayer);
        if (!destination.data() || destinationPitch < mip.RowBytes || destination.size() < mip.RowBytes || (mip.Height - 1) > (destination.size() - mip.RowBytes) / destinationPitch) throw std::invalid_argument("Texture readback does not contain all destination rows");
        const auto sourceAddress = reinterpret_cast<std::uintptr_t>(storage.Data());
        const auto destinationAddress = reinterpret_cast<std::uintptr_t>(destination.data());
        if ((destinationAddress >= sourceAddress && destinationAddress - sourceAddress < storage.Size()) || (sourceAddress >= destinationAddress && sourceAddress - destinationAddress < destination.size())) throw std::invalid_argument("Texture read destination overlaps its storage");
        const auto* source = range.data();
        for (std::size_t row = 0; row < mip.Height; ++row) std::memcpy(destination.data() + row * destinationPitch, source + row * mip.RowPitch, mip.RowBytes);
    }

    const LinearTexture2D::MipLayout& LinearTexture2D::GetMipLayout(std::uint32_t mipLevel) const
    {
        if (mipLevel >= mipLayouts.size()) throw std::out_of_range("Texture mip level exceeds its allocation");
        return mipLayouts[mipLevel];
    }

    std::uint32_t LinearTexture2D::GetMipCount() const { return static_cast<std::uint32_t>(mipLayouts.size()); }

    std::span<std::byte> LinearTexture2D::GetMipRange(std::uint32_t mipLevel, std::uint32_t arrayLayer) const
    {
        const auto& mip = GetMipLayout(mipLevel);
        if (arrayLayer >= arrayLayers) throw std::out_of_range("Texture array layer exceeds its allocation");
        return storage.GetRange(arrayLayer * layerPitch + mip.Offset, mip.Size);
    }

    const std::array<std::uint32_t, 8>& LinearTexture2D::GetDescriptor() const { return descriptor; }
    std::array<std::uint32_t, 8> LinearTexture2D::CreateViewDescriptor(bool disableSrgb, std::uint32_t firstMip, std::uint32_t mipCount, std::uint32_t arrayLayer) const
    {
        if (firstMip >= GetMipCount()) throw std::out_of_range("Texture view starts past the mip chain");
        if (mipCount == 0) mipCount = GetMipCount() - firstMip;
        if (mipCount > GetMipCount() - firstMip) throw std::out_of_range("Texture view exceeds the mip chain");
        if (arrayLayer >= arrayLayers) throw std::out_of_range("Texture view layer exceeds its allocation");
        auto result = CreateDescriptor(width, height, format, storage.GetRange(arrayLayer * layerPitch, layerPitch).data(), disableSrgb, GetMipCount());
        result[3] = (result[3] & ~0xff000u) | (firstMip << 12) | ((firstMip + mipCount - 1) << 16);
        return result;
    }

    std::array<std::uint32_t, 8> LinearTexture2D::CreateArrayViewDescriptor(bool disableSrgb, std::uint32_t firstLayer, std::uint32_t count, std::uint32_t firstMip, std::uint32_t mipCount) const
    {
        if (firstLayer >= arrayLayers || count == 0 || count > arrayLayers - firstLayer) throw std::out_of_range("Texture array view exceeds its layers");
        auto result = CreateViewDescriptor(disableSrgb, firstMip, mipCount);
        result[3] = (result[3] & 0x0fffffffu) | (13u << 28);
        result[4] = (firstLayer + count - 1) | (firstLayer << 16);
        return result;
    }

    std::array<std::uint32_t, 8> LinearTexture2D::CreateCubeViewDescriptor(bool disableSrgb, std::uint32_t firstCube, std::uint32_t cubeCount, std::uint32_t firstMip, std::uint32_t mipCount) const
    {
        if (width != height || arrayLayers % 6 != 0) throw std::invalid_argument("Cube view requires square faces in groups of six");
        const auto cubes = arrayLayers / 6;
        if (firstCube >= cubes || cubeCount == 0 || cubeCount > cubes - firstCube) throw std::out_of_range("Cube view exceeds its face allocation");
        auto result = CreateArrayViewDescriptor(disableSrgb, firstCube * 6, cubeCount * 6, firstMip, mipCount);
        result[3] = (result[3] & 0x0fffffffu) | (11u << 28);
        return result;
    }

    std::uint32_t LinearTexture2D::GetArrayLayers() const { return arrayLayers; }
    std::size_t LinearTexture2D::GetLayerPitch() const { return layerPitch; }

    const BufferAllocation& LinearTexture2D::GetStorage() const { return storage; }
    std::size_t LinearTexture2D::GetRowPitch() const { return rowPitch; }
    std::size_t LinearTexture2D::GetRowBytes() const { return rowBytes; }
    std::uint32_t LinearTexture2D::GetWidth() const { return width; }
    std::uint32_t LinearTexture2D::GetHeight() const { return height; }
    TextureFormat LinearTexture2D::GetFormat() const { return format; }
}
