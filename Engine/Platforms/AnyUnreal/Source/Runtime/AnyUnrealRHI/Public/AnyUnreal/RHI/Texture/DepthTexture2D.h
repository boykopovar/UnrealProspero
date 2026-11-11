#ifndef ANYUNREAL_RHI_DEPTH_TEXTURE_2D_H
#define ANYUNREAL_RHI_DEPTH_TEXTURE_2D_H

#include "AnyUnreal/RHI/Memory/BufferAllocation.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API DepthTexture2D final
    {
    public:
        static std::size_t CalculateDataSize(std::uint32_t width, std::uint32_t height);
        DepthTexture2D(std::uint32_t width, std::uint32_t height, std::uint32_t addressHigh);
        const BufferAllocation& GetStorage() const;
        std::uint32_t GetWidth() const;
        std::uint32_t GetHeight() const;

    private:
        std::uint32_t width;
        std::uint32_t height;
        BufferAllocation storage;
    };
}

#endif
