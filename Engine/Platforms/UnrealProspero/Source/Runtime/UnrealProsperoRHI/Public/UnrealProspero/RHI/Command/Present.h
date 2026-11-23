#ifndef UNREALPROSPERO_RHI_COMMAND_PRESENT_H
#define UNREALPROSPERO_RHI_COMMAND_PRESENT_H

#include "SceTypes.hpp"
#include <cstdint>

namespace UnrealProspero::Video
{
    class Output;
}

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API void WritePresent(CommandBuffer& commands, Video::Output& output, int bufferIndex, int mode, std::int64_t argument);
}

#endif
