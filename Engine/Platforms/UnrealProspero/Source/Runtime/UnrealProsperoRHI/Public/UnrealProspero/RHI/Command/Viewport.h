#ifndef UNREALPROSPERO_RHI_VIEWPORT_H
#define UNREALPROSPERO_RHI_VIEWPORT_H

#include "SceTypes.hpp"
#include <cstdint>

namespace UnrealProspero::RHI
{
    struct Viewport
    {
        float MinX;
        float MinY;
        float MinDepth;
        float MaxX;
        float MaxY;
        float MaxDepth;
    };

    struct ScissorRect
    {
        std::uint32_t MinX;
        std::uint32_t MinY;
        std::uint32_t MaxX;
        std::uint32_t MaxY;
    };

    UNREALPROSPERORHI_API void WriteViewport(CommandBuffer& commands, std::uint32_t index, const Viewport& viewport);
    UNREALPROSPERORHI_API void WriteViewportScissor(CommandBuffer& commands, std::uint32_t index, const ScissorRect& rectangle);
    UNREALPROSPERORHI_API void WriteScissor(CommandBuffer& commands, const ScissorRect& rectangle);
}

#endif
