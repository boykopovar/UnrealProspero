#ifndef ANYUNREAL_RHI_TEXTURE_INITIALIZER_H
#define ANYUNREAL_RHI_TEXTURE_INITIALIZER_H

#include "RHITextureInitializer.h"
#include <cstdint>

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API FRHITextureInitializer CreateTextureInitializer(FRHICommandListBase& commandList, const FRHITextureCreateDesc& description, std::uint32_t addressHigh);
}

#endif
