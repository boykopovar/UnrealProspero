#ifndef UNREALPROSPERO_RHI_TEXTURE_INITIALIZER_H
#define UNREALPROSPERO_RHI_TEXTURE_INITIALIZER_H

#include "RHITextureInitializer.h"
#include <cstdint>

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API FRHITextureInitializer CreateTextureInitializer(FRHICommandListBase& commandList, const FRHITextureCreateDesc& description, std::uint32_t addressHigh);
}

#endif
