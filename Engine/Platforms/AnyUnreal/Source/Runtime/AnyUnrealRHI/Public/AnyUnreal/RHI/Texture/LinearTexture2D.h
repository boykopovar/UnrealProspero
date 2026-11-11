#ifndef ANYUNREAL_RHI_LINEAR_TEXTURE_2D_H
#define ANYUNREAL_RHI_LINEAR_TEXTURE_2D_H

#include "AnyUnreal/RHI/Memory/BufferAllocation.h"
#include <array>

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
        Rgba32Float
    };

    class ANYUNREALRHI_API LinearTexture2D final
    {
    public:
        static std::size_t CalculateRowPitch(std::uint32_t width, TextureFormat format);
        static std::size_t CalculateDataSize(std::uint32_t width, std::uint32_t height, TextureFormat format);
        LinearTexture2D(std::uint32_t width, std::uint32_t height, TextureFormat format, std::uint32_t addressHigh);
        void Upload(std::span<const std::byte> source, std::size_t sourcePitch);
        void CopyRowsTo(std::span<std::byte> destination, std::size_t destinationPitch) const;
        const std::array<std::uint32_t, 8>& GetDescriptor() const;
        std::array<std::uint32_t, 8> CreateViewDescriptor(bool disableSrgb) const;
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
        BufferAllocation storage;
        std::array<std::uint32_t, 8> descriptor;
    };
}

#endif
