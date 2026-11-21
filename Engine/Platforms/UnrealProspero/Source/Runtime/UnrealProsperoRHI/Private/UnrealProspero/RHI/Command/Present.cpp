#include "UnrealProspero/RHI/Command/Present.h"
#include "UnrealProspero/Video/Output.h"
#include "prx/libSceAgc/DcbState/include/Display.hpp"
#include <stdexcept>

namespace UnrealProspero::RHI
{
    void WritePresent(CommandBuffer& commands, Video::Output& output, int bufferIndex, int mode, std::int64_t argument)
    {
        if (!sceAgcDcbSetFlip(&commands, static_cast<std::uint32_t>(output.GetHandle()), bufferIndex, static_cast<std::uint32_t>(mode), argument)) throw std::runtime_error("AGC could not encode the display flip");
    }
}
