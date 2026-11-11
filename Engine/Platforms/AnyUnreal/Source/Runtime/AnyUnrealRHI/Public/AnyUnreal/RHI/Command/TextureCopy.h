#ifndef ANYUNREAL_RHI_COMMAND_TEXTURE_COPY_H
#define ANYUNREAL_RHI_COMMAND_TEXTURE_COPY_H

#include "AnyUnreal/RHI/Texture/LinearTexture2D.h"
#include "AnyUnreal/RHI/Texture/LinearTexture3D.h"
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

    struct VolumeCopyRegion
    {
        std::array<std::uint32_t, 3> Source;
        std::array<std::uint32_t, 3> Destination;
        std::array<std::uint32_t, 3> Extent;
    };

    ANYUNREALRHI_API void WriteTextureCopy(CommandBuffer& commands, const LinearTexture3D& destination, const LinearTexture3D& source, const VolumeCopyRegion& region);
    ANYUNREALRHI_API void WriteTextureCopy(CommandBuffer& commands, const LinearTexture2D& destination, const LinearTexture2D& source, const TextureCopyRegion& region);
}

#endif
