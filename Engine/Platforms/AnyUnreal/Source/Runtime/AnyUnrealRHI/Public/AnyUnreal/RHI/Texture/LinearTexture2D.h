#ifndef ANYUNREAL_RHI_LINEAR_TEXTURE_2D_H
#define ANYUNREAL_RHI_LINEAR_TEXTURE_2D_H

#include "AnyUnreal/RHI/Memory/BufferAllocation.h"
#include <array>
#include <vector>

namespace AnyUnreal::RHI
{
    enum class TextureFormat
    {
        R8Unorm,
        Rg8Unorm,
        Rgba8Unorm,
        Rgba8Srgb,
        Bgra8Unorm,
        Bgra8Srgb,
        R32Float,
        Rgba32Float,
        Rgba16Float,
        R16Float,
        Rg16Float
    };

    class ANYUNREALRHI_API LinearTexture2D final
    {
    public:
        struct MipLayout
        {
            std::uint32_t Width;
            std::uint32_t Height;
            std::size_t RowBytes;
            std::size_t RowPitch;
            std::size_t Offset;
            std::size_t Size;
        };

        static std::vector<MipLayout> CalculateMipLayout(std::uint32_t width, std::uint32_t height, TextureFormat format, std::uint32_t mipCount);
        static std::size_t CalculateRowBytes(std::uint32_t width, TextureFormat format);
        static std::array<std::uint32_t, 8> CreateDescriptor(std::uint32_t width, std::uint32_t height, TextureFormat format, const void* data, bool disableSrgb = false, std::uint32_t mipCount = 1);
        static std::size_t CalculateRowPitch(std::uint32_t width, TextureFormat format);
        static std::size_t CalculateDataSize(std::uint32_t width, std::uint32_t height, TextureFormat format, std::uint32_t mipCount = 1, std::uint32_t firstMip = 0);
        LinearTexture2D(std::uint32_t width, std::uint32_t height, TextureFormat format, std::uint32_t addressHigh, std::size_t alignment = Kernel::DirectAllocation::PageSize, std::uint32_t mipCount = 1, AllocationUsage usage = AllocationUsage::Texture);
        void Upload(std::span<const std::byte> source, std::size_t sourcePitch, std::uint32_t mipLevel = 0);
        void CopyRowsTo(std::span<std::byte> destination, std::size_t destinationPitch, std::uint32_t mipLevel = 0) const;
        const std::array<std::uint32_t, 8>& GetDescriptor() const;
        std::array<std::uint32_t, 8> CreateViewDescriptor(bool disableSrgb, std::uint32_t firstMip = 0, std::uint32_t mipCount = 0) const;
        const MipLayout& GetMipLayout(std::uint32_t mipLevel) const;
        std::uint32_t GetMipCount() const;
        std::span<std::byte> GetMipRange(std::uint32_t mipLevel) const;
        const BufferAllocation& GetStorage() const;
        std::size_t GetRowPitch() const;
        std::size_t GetRowBytes() const;
        std::uint32_t GetWidth() const;
        std::uint32_t GetHeight() const;
        TextureFormat GetFormat() const;

    private:
        std::uint32_t width;
        std::uint32_t height;
        TextureFormat format;
        std::size_t rowBytes;
        std::size_t rowPitch;
        std::vector<MipLayout> mipLayouts;
        BufferAllocation storage;
        std::array<std::uint32_t, 8> descriptor;
    };
}

#endif
