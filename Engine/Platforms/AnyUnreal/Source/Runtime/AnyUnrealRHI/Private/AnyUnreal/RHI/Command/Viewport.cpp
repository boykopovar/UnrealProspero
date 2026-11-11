#include "AnyUnreal/RHI/Command/Viewport.h"
#include "prx/libSceAgc/DcbState/include/Registers.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace AnyUnreal::RHI
{
    namespace
    {
        void checkIndex(std::uint32_t index)
        {
            if (index >= 16) throw std::out_of_range("Viewport index exceeds the hardware limit");
        }

        void writeRectangle(CommandBuffer& commands, std::uint32_t offset, const ScissorRect& rectangle)
        {
            if (rectangle.MinX > rectangle.MaxX || rectangle.MinY > rectangle.MaxY || rectangle.MaxX > 32767 || rectangle.MaxY > 32767) throw std::out_of_range("Scissor rectangle exceeds the hardware bounds");
            sceAgcDcbSetCxRegisterDirect(&commands, {offset, rectangle.MinX | (rectangle.MinY << 16) | (1u << 31)});
            sceAgcDcbSetCxRegisterDirect(&commands, {offset + 1, rectangle.MaxX | (rectangle.MaxY << 16)});
        }
    }

    void WriteViewport(CommandBuffer& commands, std::uint32_t index, const Viewport& viewport)
    {
        checkIndex(index);
        const std::array values{viewport.MinX, viewport.MinY, viewport.MinDepth, viewport.MaxX, viewport.MaxY, viewport.MaxDepth};
        if (!std::all_of(values.begin(), values.end(), [](float value) { return std::isfinite(value); })) throw std::invalid_argument("Viewport coordinates must be finite");
        if (viewport.MaxX < viewport.MinX || viewport.MaxY < viewport.MinY || viewport.MinDepth < 0 || viewport.MinDepth > 1 || viewport.MaxDepth < 0 || viewport.MaxDepth > 1) throw std::out_of_range("Invalid viewport extent or depth range");
        const auto scaleX = (viewport.MaxX - viewport.MinX) * 0.5f;
        const auto scaleY = (viewport.MaxY - viewport.MinY) * 0.5f;
        const std::array transform{scaleX, viewport.MinX + scaleX, scaleY, viewport.MinY + scaleY, viewport.MaxDepth - viewport.MinDepth, viewport.MinDepth};
        if (!std::all_of(transform.begin(), transform.end(), [](float value) { return std::isfinite(value); })) throw std::out_of_range("Viewport transform exceeds floating-point range");
        for (std::uint32_t component = 0; component < transform.size(); ++component) sceAgcDcbSetCxRegisterDirect(&commands, {0x10fu + index * 6 + component, std::bit_cast<std::uint32_t>(transform[component])});
        sceAgcDcbSetCxRegisterDirect(&commands, {0xb4u + index * 2, std::bit_cast<std::uint32_t>(std::min(viewport.MinDepth, viewport.MaxDepth))});
        sceAgcDcbSetCxRegisterDirect(&commands, {0xb5u + index * 2, std::bit_cast<std::uint32_t>(std::max(viewport.MinDepth, viewport.MaxDepth))});
    }

    void WriteViewportScissor(CommandBuffer& commands, std::uint32_t index, const ScissorRect& rectangle)
    {
        checkIndex(index);
        writeRectangle(commands, 0x94u + index * 2, rectangle);
    }

    void WriteScissor(CommandBuffer& commands, const ScissorRect& rectangle)
    {
        writeRectangle(commands, 0x90, rectangle);
    }
}
