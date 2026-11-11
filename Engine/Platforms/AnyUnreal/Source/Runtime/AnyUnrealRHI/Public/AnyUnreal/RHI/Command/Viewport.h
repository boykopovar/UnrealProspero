#ifndef ANYUNREAL_RHI_VIEWPORT_H
#define ANYUNREAL_RHI_VIEWPORT_H

#include "SceTypes.hpp"
#include <cstdint>

namespace AnyUnreal::RHI
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

    ANYUNREALRHI_API void WriteViewport(CommandBuffer& commands, std::uint32_t index, const Viewport& viewport);
    ANYUNREALRHI_API void WriteViewportScissor(CommandBuffer& commands, std::uint32_t index, const ScissorRect& rectangle);
    ANYUNREALRHI_API void WriteScissor(CommandBuffer& commands, const ScissorRect& rectangle);
}

#endif
