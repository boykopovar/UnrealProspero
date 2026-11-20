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
        sceAgcCbReleaseMem(&commands, 4, 0x200, 1, 3, label, 2, 1, 0, 0, 2, 0);
        if (commands.cursor_up - commands.bottom != offset + 8) throw std::runtime_error("Unexpected end of pipe flip packet size");
        auto* packet = commands.bottom + offset;
        if (packet[0] != 0xc0064900u || packet[1] != 0x06200504u || packet[2] != 0x42010000u) throw std::runtime_error("Unexpected end of pipe flip packet layout");
        const auto token = output.ReserveEopFlip(bufferIndex, mode, argument);
        packet[7] = token;
    }
}
