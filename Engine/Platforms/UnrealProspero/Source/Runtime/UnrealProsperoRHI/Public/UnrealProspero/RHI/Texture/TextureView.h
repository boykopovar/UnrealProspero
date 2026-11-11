#ifndef UNREALPROSPERO_RHI_TEXTURE_VIEW_H
#define UNREALPROSPERO_RHI_TEXTURE_VIEW_H

#include "UnrealProspero/RHI/Texture/TextureResource.h"

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API TextureShaderResourceView final : public FRHIShaderResourceView
    {
    public:
        TextureShaderResourceView(TextureResource& texture, const FRHIViewDesc& description);
        const std::array<std::uint32_t, 8>& GetDescriptor() const { return descriptor; }
        const BufferAllocation* GetStorage() const { return storage.get(); }

    private:
        std::shared_ptr<const BufferAllocation> storage;
        std::array<std::uint32_t, 8> descriptor;
    };
    class UNREALPROSPERORHI_API TextureUnorderedAccessView final : public FRHIUnorderedAccessView
    {
    public:
        TextureUnorderedAccessView(TextureResource& texture, const FRHIViewDesc& description);
        const std::array<std::uint32_t, 8>& GetDescriptor() const { return descriptor; }
        const BufferAllocation* GetStorage() const { return storage.get(); }
        std::shared_ptr<const BufferAllocation> CaptureStorage() const { return storage; }
        TextureFormat GetFormat() const { return format; }
        std::span<const std::span<std::byte>> GetRanges() const { return ranges; }

    private:
        TextureFormat format;
        std::shared_ptr<const BufferAllocation> storage;
        std::array<std::uint32_t, 8> descriptor;
        std::vector<std::span<std::byte>> ranges;
    };
}

#endif
