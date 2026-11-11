#ifndef ANYUNREAL_RHI_TEXTURE_RESOURCE_H
#define ANYUNREAL_RHI_TEXTURE_RESOURCE_H

#include "AnyUnreal/RHI/Texture/LinearTexture2D.h"
#include "AnyUnreal/RHI/Texture/LinearTexture3D.h"
#include "AnyUnreal/RHI/Texture/DepthTexture2D.h"
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

        static std::size_t CalculateAlignment(const FRHITextureDesc& description);
        static std::size_t CalculateAllocationSize(const FRHITextureDesc& description);
        TextureResource(const FRHITextureCreateDesc& description, std::uint32_t addressHigh);
        TextureResource(const FRHITextureCreateDesc& description, std::uint32_t addressHigh, const void* initialData);
        TextureResource(const FRHITextureCreateDesc& description, std::shared_ptr<LinearTexture2D> externalStorage);
        void* Lock(EResourceLockMode mode, std::size_t& size, std::size_t& pitch);
        LockedData Unlock();
        void* MapReadback();
        void UnmapReadback();
        std::shared_ptr<const LinearTexture2D> Capture() const;
        std::shared_ptr<const DepthTexture2D> CaptureDepth() const;
        std::shared_ptr<const LinearTexture3D> CaptureVolume() const;
        std::shared_ptr<const BufferAllocation> CaptureStorage() const;
        std::array<std::uint32_t, 8> CreateViewDescriptor(bool disableSrgb, std::uint32_t firstMip = 0, std::uint32_t mipCount = 0) const;
        std::span<std::byte> GetMipRange(std::uint32_t mipLevel) const;
        const BufferAllocation& GetStorage() const;
        std::size_t GetAllocationSize() const;

    private:
        friend class TextureInitializer;
        void finishInitialization();
        const BufferAllocation& getAllocation() const;
        std::size_t getRowPitch() const;
        void uploadInitialData(std::span<const std::byte> data);
        FRHITextureSubresourceInitializer getSubresource(FRHITextureInitializer::FSubresourceIndex index);
        std::shared_ptr<LinearTexture2D> storage;
        std::shared_ptr<DepthTexture2D> depthStorage;
        std::shared_ptr<LinearTexture3D> volumeStorage;
        bool initialized;
        FCriticalSection lockMutex;
        std::optional<LockedData> lockedData;
        std::optional<std::vector<std::byte>> mappedData;
    };

    ANYUNREALRHI_API TextureResource& ResolveTextureResource(FRHIResource* resource);
}

#endif
