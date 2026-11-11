#ifndef ANYUNREAL_RHI_GRAPHICS_CONTEXT_H
#define ANYUNREAL_RHI_GRAPHICS_CONTEXT_H

#include "AnyUnreal/RHI/Context/ComputeContext.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API GraphicsContext : public ComputeContext
    {
    public:
        void RHISetViewport(float minX, float minY, float minZ, float maxX, float maxY, float maxZ) override;
        void RHISetMultipleViewports(uint32 count, const FViewportBounds* data) override;
        void RHISetScissorRect(bool enable, uint32 minX, uint32 minY, uint32 maxX, uint32 maxY) override;

    protected:
        GraphicsContext(std::size_t initialCommandBytes, std::uint32_t addressHigh);
    };
}

#endif
