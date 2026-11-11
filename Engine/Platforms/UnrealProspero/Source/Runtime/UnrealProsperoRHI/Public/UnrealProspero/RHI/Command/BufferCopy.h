#ifndef UNREALPROSPERO_RHI_BUFFER_COPY_H
#define UNREALPROSPERO_RHI_BUFFER_COPY_H

#include "SceTypes.hpp"
#include <cstddef>

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API void WriteBufferCopy(CommandBuffer& commands, void* destination, const void* source, std::size_t size);
}

#endif
