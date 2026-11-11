#ifndef ANYUNREAL_RHI_TEXTURE_UPLOAD_H
#define ANYUNREAL_RHI_TEXTURE_UPLOAD_H

#include "AnyUnreal/RHI/Command/TextureCopy.h"
#include "AnyUnreal/RHI/Command/MemoryBarrier.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API TextureUpload final
    {
    public:
        TextureUpload(std::uint32_t width, std::uint32_t height, TextureFormat format, std::span<const std::byte> source, std::size_t sourcePitch, std::uint32_t addressHigh);
        void Record(CommandBuffer& commands, const LinearTexture2D& destination, std::uint32_t x, std::uint32_t y);

    private:
        LinearTexture2D storage;
        MemoryBarrier barrier;
        bool recorded = false;
    };
}

#endif
