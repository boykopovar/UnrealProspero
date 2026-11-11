#ifndef UNREALPROSPERO_RHI_SURFACE_DATA_H
#define UNREALPROSPERO_RHI_SURFACE_DATA_H

#include "UnrealProspero/RHI/Texture/LinearTexture2D.h"
#include "UnrealProspero/RHI/Texture/LinearTexture3D.h"
#include "Containers/Array.h"
#include "Math/Color.h"
#include "Math/Float16Color.h"
#include "Math/IntRect.h"
#include "RHIResources.h"
#include "RHITypes.h"

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API void ReadVolumeHalfColors(const LinearTexture3D& texture, FIntRect rectangle, FIntPoint zRange, TArray<FFloat16Color>& output);
    UNREALPROSPERORHI_API void ReadSurfaceHalfColors(const LinearTexture2D& texture, FIntRect rectangle, TArray<FFloat16Color>& output, std::uint32_t mipLevel = 0, std::uint32_t arrayLayer = 0);
    UNREALPROSPERORHI_API void ReadSurfaceColors(const LinearTexture2D& texture, FIntRect rectangle, const FReadSurfaceDataFlags& flags, TArray<FColor>& output);
    UNREALPROSPERORHI_API void ReadSurfaceColors(const LinearTexture2D& texture, FIntRect rectangle, const FReadSurfaceDataFlags& flags, TArray<FLinearColor>& output);
}

#endif
