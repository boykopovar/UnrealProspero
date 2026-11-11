#include "AnyUnreal/RHI/Context/GraphicsContext.h"
#include "AnyUnreal/RHI/Command/Viewport.h"
#include "RHI.h"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    GraphicsContext::GraphicsContext(std::size_t initialCommandBytes, std::uint32_t addressHigh) : ComputeContext(initialCommandBytes, addressHigh)
    {
        RecordCommands([](CommandBuffer& commands) { WriteScissor(commands, {0, 0, 32767, 32767}); });
    }

    void GraphicsContext::RHISetViewport(float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
    {
        RecordCommands([&](CommandBuffer& commands)
        {
            WriteViewport(commands, 0, {minX, minY, minZ, maxX, maxY, maxZ});
            WriteViewportScissor(commands, 0, {0, 0, 32767, 32767});
        });
    }

    void GraphicsContext::RHISetMultipleViewports(uint32 count, const FViewportBounds* data)
    {
        RecordCommands([&](CommandBuffer& commands)
        {
            if (!data || count == 0 || count > 16) throw std::invalid_argument("Multiple viewports require one to sixteen bounds");
            for (uint32 index = 0; index < count; ++index)
            {
                const auto& viewport = data[index];
                if (viewport.Width < 0 || viewport.Height < 0) throw std::out_of_range("Viewport dimensions must be nonnegative");
                WriteViewport(commands, index, {viewport.TopLeftX, viewport.TopLeftY, viewport.MinDepth, viewport.TopLeftX + viewport.Width, viewport.TopLeftY + viewport.Height, viewport.MaxDepth});
                WriteViewportScissor(commands, index, {0, 0, 32767, 32767});
            }
        });
    }

    void GraphicsContext::RHISetScissorRect(bool enable, uint32 minX, uint32 minY, uint32 maxX, uint32 maxY)
    {
        RecordCommands([&](CommandBuffer& commands)
        {
            WriteScissor(commands, enable ? ScissorRect{minX, minY, maxX, maxY} : ScissorRect{0, 0, 32767, 32767});
        });
    }
}
