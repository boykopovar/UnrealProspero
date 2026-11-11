#ifndef ANYUNREAL_RHI_TEXTURE_RESOURCE_H
#define ANYUNREAL_RHI_TEXTURE_RESOURCE_H

#include "AnyUnreal/RHI/Texture/LinearTexture2D.h"
#include "RHITextureInitializer.h"
#include <memory>

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API TextureResource final : public FRHITexture
    {
    public:
        TextureResource(const FRHITextureCreateDesc& description, std::uint32_t addressHigh);
        std::shared_ptr<const LinearTexture2D> Capture() const;

    private:
        friend class TextureInitializer;
        void finishInitialization();
        FRHITextureSubresourceInitializer getSubresource(FRHITextureInitializer::FSubresourceIndex index);
        std::shared_ptr<LinearTexture2D> storage;
        bool initialized;
    };
}

#endif
