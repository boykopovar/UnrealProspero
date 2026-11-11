#ifndef ANYUNREAL_RHI_TEXTURE_RESOURCE_H
#define ANYUNREAL_RHI_TEXTURE_RESOURCE_H

#include "AnyUnreal/RHI/Texture/LinearTexture2D.h"
#include "RHITextureInitializer.h"
#include "HAL/CriticalSection.h"
#include <memory>
#include <optional>
#include <vector>

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API TextureResource final : public FRHITexture
    {
    public:
        struct LockedData
        {
            EResourceLockMode Mode;
            std::size_t Pitch;
            std::vector<std::byte> Data;
        };

        TextureResource(const FRHITextureCreateDesc& description, std::uint32_t addressHigh);
        void* Lock(EResourceLockMode mode, std::size_t& size, std::size_t& pitch);
        LockedData Unlock();
        std::shared_ptr<const LinearTexture2D> Capture() const;
        std::size_t GetAllocationSize() const;

    private:
        friend class TextureInitializer;
        void finishInitialization();
        FRHITextureSubresourceInitializer getSubresource(FRHITextureInitializer::FSubresourceIndex index);
        std::shared_ptr<LinearTexture2D> storage;
        bool initialized;
        FCriticalSection lockMutex;
        std::optional<LockedData> lockedData;
    };

    ANYUNREALRHI_API TextureResource& ResolveTextureResource(FRHIResource* resource);
}

#endif
