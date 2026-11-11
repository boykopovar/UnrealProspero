#ifndef ANYUNREAL_RHI_LINEAR_TEXTURE_3D_H
#define ANYUNREAL_RHI_LINEAR_TEXTURE_3D_H

#include "AnyUnreal/RHI/Texture/LinearTexture2D.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API LinearTexture3D final
    {
    public:
        static std::size_t CalculateDataSize(std::uint32_t width, std::uint32_t height, std::uint32_t depth, TextureFormat format, std::uint32_t mipCount = 1);
        LinearTexture3D(std::uint32_t width, std::uint32_t height, std::uint32_t depth, TextureFormat format, std::uint32_t addressHigh, std::uint32_t mipCount = 1);
        void Upload(std::span<const std::byte> source, std::size_t sourceRowPitch, std::size_t sourceSlicePitch, std::uint32_t mipLevel = 0);
        void CopyTo(std::span<std::byte> destination, std::size_t destinationRowPitch, std::size_t destinationSlicePitch, std::uint32_t mipLevel = 0) const;
        std::array<std::uint32_t, 8> GetDescriptor() const;
        std::array<std::uint32_t, 8> CreateViewDescriptor(bool disableSrgb, std::uint32_t firstMip = 0, std::uint32_t mipCount = 0) const;
        const LinearMipLayout& GetMipLayout(std::uint32_t mipLevel) const;
        std::uint32_t GetMipCount() const;
        std::span<std::byte> GetSliceRange(std::uint32_t mipLevel, std::uint32_t slice) const;
        const BufferAllocation& GetStorage() const;
        std::size_t GetRowPitch() const;
        std::size_t GetSlicePitch() const;
        std::uint32_t GetWidth() const;
        std::uint32_t GetHeight() const;
        std::uint32_t GetDepth() const;
        TextureFormat GetFormat() const;

    private:
        std::size_t validateLayout(std::size_t size, std::size_t dataRowPitch, std::size_t dataSlicePitch, const LinearMipLayout& mip) const;
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t depth;
        TextureFormat format;
        std::vector<LinearMipLayout> mipLayouts;
        std::size_t rowPitch;
        std::size_t slicePitch;
        BufferAllocation storage;
    };
}

#endif
