#ifndef UNREALPROSPERO_RHI_BUFFER_INITIALIZER_H
#define UNREALPROSPERO_RHI_BUFFER_INITIALIZER_H

#include "RHIBufferInitializer.h"
#include <cstdint>

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API FRHIBufferInitializer CreateBufferInitializer(FRHICommandListBase& commandList, const FRHIBufferCreateDesc& description, std::uint32_t addressHigh);
}

#endif
