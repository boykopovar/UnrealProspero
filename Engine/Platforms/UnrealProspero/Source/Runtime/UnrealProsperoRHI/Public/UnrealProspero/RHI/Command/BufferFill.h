#ifndef UNREALPROSPERO_RHI_BUFFER_FILL_H
#define UNREALPROSPERO_RHI_BUFFER_FILL_H

#include "SceTypes.hpp"
#include <cstddef>
#include <cstdint>

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API void WriteBufferFill(CommandBuffer& commands, void* destination, std::size_t size, std::uint32_t value);
}

#endif
