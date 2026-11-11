#ifndef UNREALPROSPERO_RHI_BUFFER_CLEAR_VALUE_H
#define UNREALPROSPERO_RHI_BUFFER_CLEAR_VALUE_H

#include "UnrealProspero/RHI/Descriptor/BufferDescriptor.h"

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API std::array<std::uint32_t, 4> EncodeBufferClear(BufferFormat format, const std::array<std::uint32_t, 4>& values);
    UNREALPROSPERORHI_API std::array<std::uint32_t, 4> EncodeBufferClear(BufferFormat format, const std::array<float, 4>& values);
}

#endif
