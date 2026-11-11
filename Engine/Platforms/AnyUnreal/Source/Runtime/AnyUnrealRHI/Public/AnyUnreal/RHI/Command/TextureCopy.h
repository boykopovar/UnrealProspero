#ifndef ANYUNREAL_RHI_COMMAND_TEXTURE_COPY_H
#define ANYUNREAL_RHI_COMMAND_TEXTURE_COPY_H

#include "AnyUnreal/RHI/Texture/LinearTexture2D.h"
#include "SceTypes.hpp"

namespace AnyUnreal::RHI
{
    struct TextureCopyRegion
    {
        std::uint32_t SourceX;
        std::uint32_t SourceY;
        std::uint32_t DestinationX;
        std::uint32_t DestinationY;
        std::uint32_t Width;
        std::uint32_t Height;
    };

    ANYUNREALRHI_API void WriteTextureCopy(CommandBuffer& commands, const LinearTexture2D& destination, const LinearTexture2D& source, const TextureCopyRegion& region);
}

#endif
