#ifndef ANYUNREAL_RHI_BUFFER_INITIALIZER_H
#define ANYUNREAL_RHI_BUFFER_INITIALIZER_H

#include "RHIBufferInitializer.h"
#include <cstdint>

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API FRHIBufferInitializer CreateBufferInitializer(FRHICommandListBase& commandList, const FRHIBufferCreateDesc& description, std::uint32_t addressHigh);
}

#endif
