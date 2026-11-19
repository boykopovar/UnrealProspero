#ifndef UNREALPROSPERO_RHI_TEXTURE_2D_H
#define UNREALPROSPERO_RHI_TEXTURE_2D_H

#include "UnrealProspero/RHI/Memory/BufferAllocation.h"
#include <array>
#include <vector>
#include "UnrealProspero/RHI/Texture/LinearMipLayout.h"
#include "UnrealProspero/RHI/Texture/TextureFormat.h"

namespace UnrealProspero::RHI
{
    enum class Texture2DLayout { Linear, RenderTarget64K };

    class UNREALPROSPERORHI_API Texture2D final
    {
    public:
        using MipLayout = LinearMipLayout;

        static std::vector<MipLayout> CalculateMipLayout(std::uint32_t width, std::uint32_t height, TextureFormat format, std::uint32_t mipCount, Texture2DLayout layout = Texture2DLayout::Linear);
        static std::size_t CalculateRowBytes(std::uint32_t width, TextureFormat format);
        static std::array<std::uint32_t, 8> CreateDescriptor(std::uint32_t width, std::uint32_t height, TextureFormat format, const void* data, bool disableSrgb = false, std::uint32_t mipCount = 1, Texture2DLayout layout = Texture2DLayout::Linear);
        static std::size_t CalculateRowPitch(std::uint32_t width, TextureFormat format, Texture2DLayout layout = Texture2DLayout::Linear);
        static std::size_t CalculateDataSize(std::uint32_t width, std::uint32_t height, TextureFormat format, std::uint32_t mipCount = 1, std::uint32_t firstMip = 0, Texture2DLayout layout = Texture2DLayout::Linear);
        Texture2D(std::uint32_t width, std::uint32_t height, TextureFormat format, std::uint32_t addressHigh, std::size_t alignment = Kernel::DirectAllocation::PageSize, std::uint32_t mipCount = 1, AllocationUsage usage = AllocationUsage::Texture, std::uint32_t arrayLayers = 1, Texture2DLayout layout = Texture2DLayout::Linear);
        void Upload(std::span<const std::byte> source, std::size_t sourcePitch, std::uint32_t mipLevel = 0, std::uint32_t arrayLayer = 0);
        void CopyRowsTo(std::span<std::byte> destination, std::size_t destinationPitch, std::uint32_t mipLevel = 0, std::uint32_t arrayLayer = 0) const;
        const std::array<std::uint32_t, 8>& GetDescriptor() const;
        std::array<std::uint32_t, 8> CreateViewDescriptor(bool disableSrgb, std::uint32_t firstMip = 0, std::uint32_t mipCount = 0, std::uint32_t arrayLayer = 0) const;
        std::array<std::uint32_t, 8> CreateArrayViewDescriptor(bool disableSrgb, std::uint32_t firstLayer, std::uint32_t layerCount, std::uint32_t firstMip = 0, std::uint32_t mipCount = 0) const;
        std::array<std::uint32_t, 8> CreateCubeViewDescriptor(bool disableSrgb, std::uint32_t firstCube, std::uint32_t cubeCount, std::uint32_t firstMip = 0, std::uint32_t mipCount = 0) const;
        std::uint32_t GetArrayLayers() const;
        std::size_t GetLayerPitch() const;
        const MipLayout& GetMipLayout(std::uint32_t mipLevel) const;
        std::uint32_t GetMipCount() const;
        std::span<std::byte> GetMipRange(std::uint32_t mipLevel, std::uint32_t arrayLayer = 0) const;
        const BufferAllocation& GetStorage() const;
        std::size_t GetRowPitch() const;
        std::size_t GetRowBytes() const;
        std::uint32_t GetWidth() const;
        std::uint32_t GetHeight() const;
        TextureFormat GetFormat() const;
        Texture2DLayout GetLayout() const;

    private:
        std::size_t tiledTexelOffset(std::uint32_t x, std::uint32_t y) const;
        std::uint32_t width;
        std::uint32_t height;
        TextureFormat format;
        Texture2DLayout layout;
        std::size_t rowBytes;
        std::size_t rowPitch;
        std::vector<MipLayout> mipLayouts;
        std::uint32_t arrayLayers;
        std::size_t layerPitch;
        BufferAllocation storage;
        std::array<std::uint32_t, 8> descriptor;
    };
}

#endif
