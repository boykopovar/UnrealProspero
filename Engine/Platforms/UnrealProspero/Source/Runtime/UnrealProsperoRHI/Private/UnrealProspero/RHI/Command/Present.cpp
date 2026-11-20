#include "UnrealProspero/RHI/Command/Present.h"
#include "UnrealProspero/Video/Output.h"
#include "prx/libSceAgc/Cb/include/Sync.hpp"
#include <stdexcept>

namespace UnrealProspero::RHI
{
    void WritePresent(CommandBuffer& commands, Video::Output& output, int bufferIndex, int mode, std::int64_t argument)
    {
        auto* label = reinterpret_cast<Label*>(&output.GetBufferLabel(bufferIndex));
        const auto offset = commands.cursor_up - commands.bottom;
        sceAgcCbReleaseMem(&commands, 4, 0x620, 1, 0, label, 2, 1, 0, 0, 2, 0);
        if (commands.cursor_up - commands.bottom != offset + 8) throw std::runtime_error("Unexpected end of pipe flip packet size");
        const auto token = output.ReserveEopFlip(bufferIndex, mode, argument);
        commands.bottom[offset + 7] = token;
    }
}
