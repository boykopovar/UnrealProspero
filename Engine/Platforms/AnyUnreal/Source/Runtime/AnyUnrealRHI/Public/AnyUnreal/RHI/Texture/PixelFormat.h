#ifndef ANYUNREAL_RHI_TEXTURE_PIXEL_FORMAT_H
#define ANYUNREAL_RHI_TEXTURE_PIXEL_FORMAT_H

#include "AnyUnreal/RHI/Texture/LinearTexture2D.h"
#include <PixelFormat.h>

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API TextureFormat ResolveTextureFormat(EPixelFormat format, bool srgb);
    ANYUNREALRHI_API void ConfigurePixelFormats();
}

#endif
