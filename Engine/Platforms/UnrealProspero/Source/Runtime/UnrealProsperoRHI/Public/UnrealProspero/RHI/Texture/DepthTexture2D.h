#ifndef UNREALPROSPERO_RHI_DEPTH_TEXTURE_2D_H
#define UNREALPROSPERO_RHI_DEPTH_TEXTURE_2D_H

#include "UnrealProspero/RHI/Memory/BufferAllocation.h"
#include <array>

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API DepthTexture2D final
    {
    public:
        static std::size_t CalculateDataSize(std::uint32_t width, std::uint32_t height, bool stencil = false);
        DepthTexture2D(std::uint32_t width, std::uint32_t height, std::uint32_t addressHigh, bool stencil = false);
        std::array<std::uint32_t, 8> GetDescriptor(bool stencil = false) const;
        bool HasStencil() const;
        std::span<std::byte> GetDepthRange() const;
        std::span<std::byte> GetStencilRange() const;
        const BufferAllocation& GetStorage() const;
        std::uint32_t GetWidth() const;
        std::uint32_t GetHeight() const;

    private:
        std::uint32_t width;
        std::uint32_t height;
        bool stencilEnabled;
        BufferAllocation storage;
    };
}

#endif
