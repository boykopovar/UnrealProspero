#ifndef ANYUNREAL_RHI_VOLUME_UPLOAD_H
#define ANYUNREAL_RHI_VOLUME_UPLOAD_H

#include "AnyUnreal/RHI/Texture/LinearTexture3D.h"
#include "AnyUnreal/RHI/Command/MemoryBarrier.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API VolumeUpload final
    {
    public:
        VolumeUpload(std::uint32_t width, std::uint32_t height, std::uint32_t depth, TextureFormat format, std::span<const std::byte> source, std::size_t rowPitch, std::size_t slicePitch, std::uint32_t addressHigh);
        void Record(CommandBuffer& commands, const LinearTexture3D& destination, std::uint32_t x, std::uint32_t y, std::uint32_t z, std::uint32_t mipLevel = 0);

    private:
        LinearTexture3D storage;
        MemoryBarrier barrier;
        bool recorded = false;
    };
}

#endif
