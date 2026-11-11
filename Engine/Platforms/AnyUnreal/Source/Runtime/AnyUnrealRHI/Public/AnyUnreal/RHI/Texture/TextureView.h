#ifndef ANYUNREAL_RHI_TEXTURE_VIEW_H
#define ANYUNREAL_RHI_TEXTURE_VIEW_H

#include "AnyUnreal/RHI/Texture/TextureResource.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API TextureShaderResourceView final : public FRHIShaderResourceView
    {
    public:
        TextureShaderResourceView(TextureResource& texture, const FRHIViewDesc& description);
        const std::array<std::uint32_t, 8>& GetDescriptor() const { return descriptor; }
        const BufferAllocation* GetStorage() const { return &storage->GetStorage(); }

    private:
        std::shared_ptr<const LinearTexture2D> storage;
        std::array<std::uint32_t, 8> descriptor;
    };
}

#endif
