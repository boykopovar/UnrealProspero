#ifndef UNREALPROSPERO_RHI_TEXTURE_PIXEL_FORMAT_H
#define UNREALPROSPERO_RHI_TEXTURE_PIXEL_FORMAT_H

#include "UnrealProspero/RHI/Texture/LinearTexture2D.h"
#include <PixelFormat.h>

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API TextureFormat ResolveTextureFormat(EPixelFormat format, bool srgb);
    UNREALPROSPERORHI_API void ConfigurePixelFormats();
}

#endif
