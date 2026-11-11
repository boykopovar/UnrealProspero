#ifndef ANYUNREAL_RHI_SURFACE_DATA_H
#define ANYUNREAL_RHI_SURFACE_DATA_H

#include "AnyUnreal/RHI/Texture/LinearTexture2D.h"
#include "AnyUnreal/RHI/Texture/LinearTexture3D.h"
#include "Containers/Array.h"
#include "Math/Color.h"
#include "Math/Float16Color.h"
#include "Math/IntRect.h"
#include "RHIResources.h"
#include "RHITypes.h"

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API void ReadVolumeHalfColors(const LinearTexture3D& texture, FIntRect rectangle, FIntPoint zRange, TArray<FFloat16Color>& output);
    ANYUNREALRHI_API void ReadSurfaceHalfColors(const LinearTexture2D& texture, FIntRect rectangle, TArray<FFloat16Color>& output, std::uint32_t mipLevel = 0, std::uint32_t arrayLayer = 0);
    ANYUNREALRHI_API void ReadSurfaceColors(const LinearTexture2D& texture, FIntRect rectangle, const FReadSurfaceDataFlags& flags, TArray<FColor>& output);
    ANYUNREALRHI_API void ReadSurfaceColors(const LinearTexture2D& texture, FIntRect rectangle, const FReadSurfaceDataFlags& flags, TArray<FLinearColor>& output);
}

#endif
