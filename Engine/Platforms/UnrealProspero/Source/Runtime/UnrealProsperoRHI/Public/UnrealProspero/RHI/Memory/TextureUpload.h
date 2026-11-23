#ifndef UNREALPROSPERO_RHI_TEXTURE_UPLOAD_H
#define UNREALPROSPERO_RHI_TEXTURE_UPLOAD_H

#include "UnrealProspero/RHI/Command/TextureCopy.h"
#include "UnrealProspero/RHI/Command/MemoryBarrier.h"

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API TextureUpload final
    {
    public:
        TextureUpload(std::uint32_t width, std::uint32_t height, TextureFormat format, std::span<const std::byte> source, std::size_t sourcePitch, std::uint32_t addressHigh);
        void Record(CommandBuffer& commands, const Texture2D& destination, std::uint32_t x, std::uint32_t y, std::uint32_t mipLevel = 0, std::uint32_t arrayLayer = 0);

    private:
        Texture2D storage;
        MemoryBarrier barrier;
        bool recorded = false;
    };
}

#endif
