#ifndef ANYUNREAL_RHI_LINEAR_TEXTURE_3D_H
#define ANYUNREAL_RHI_LINEAR_TEXTURE_3D_H

#include "AnyUnreal/RHI/Texture/LinearTexture2D.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API LinearTexture3D final
    {
    public:
        static std::size_t CalculateDataSize(std::uint32_t width, std::uint32_t height, std::uint32_t depth, TextureFormat format);
        LinearTexture3D(std::uint32_t width, std::uint32_t height, std::uint32_t depth, TextureFormat format, std::uint32_t addressHigh);
        void Upload(std::span<const std::byte> source, std::size_t sourceRowPitch, std::size_t sourceSlicePitch);
        void CopyTo(std::span<std::byte> destination, std::size_t destinationRowPitch, std::size_t destinationSlicePitch) const;
        std::array<std::uint32_t, 8> GetDescriptor() const;
        const BufferAllocation& GetStorage() const;
        std::size_t GetRowPitch() const;
        std::size_t GetSlicePitch() const;
        std::uint32_t GetWidth() const;
        std::uint32_t GetHeight() const;
        std::uint32_t GetDepth() const;

    private:
        std::size_t validateLayout(std::size_t size, std::size_t rowPitch, std::size_t slicePitch) const;
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t depth;
        TextureFormat format;
        std::size_t rowBytes;
        std::size_t rowPitch;
        std::size_t slicePitch;
        BufferAllocation storage;
    };
}

#endif
