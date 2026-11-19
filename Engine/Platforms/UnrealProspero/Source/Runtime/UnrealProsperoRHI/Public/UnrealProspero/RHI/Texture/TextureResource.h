#ifndef UNREALPROSPERO_RHI_TEXTURE_RESOURCE_H
#define UNREALPROSPERO_RHI_TEXTURE_RESOURCE_H

#include "UnrealProspero/RHI/Texture/Texture2D.h"
#include "UnrealProspero/RHI/Texture/LinearTexture3D.h"
#include "UnrealProspero/RHI/Texture/DepthTexture2D.h"
#include "RHITextureInitializer.h"
#include "HAL/CriticalSection.h"
#include <memory>
#include <map>
#include <optional>
#include <vector>
#include <utility>

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API TextureResource final : public FRHITexture
    {
    public:
        struct LockedData
        {
            EResourceLockMode Mode;
            std::uint32_t MipLevel;
            std::uint32_t ArrayLayer;
            std::size_t Pitch;
            std::vector<std::byte> Data;
        };

        static std::size_t CalculateAlignment(const FRHITextureDesc& description);
        static std::size_t CalculateAllocationSize(const FRHITextureDesc& description, std::uint32_t firstMip = 0);
        TextureResource(const FRHITextureCreateDesc& description, std::uint32_t addressHigh);
        TextureResource(const FRHITextureCreateDesc& description, std::uint32_t addressHigh, std::span<void* const> initialMipData);
        TextureResource(const FRHITextureCreateDesc& description, std::shared_ptr<Texture2D> externalStorage);
        void* Lock(EResourceLockMode mode, std::uint32_t mipLevel, std::size_t& size, std::size_t& pitch, std::uint32_t arrayLayer = 0);
        LockedData Unlock(EResourceLockMode mode, std::uint32_t mipLevel, std::uint32_t arrayLayer = 0);
        std::uint32_t GetArrayLayer(std::uint32_t arrayIndex, std::uint32_t faceIndex) const;
        void* MapReadback();
        void UnmapReadback();
        std::shared_ptr<const Texture2D> Capture() const;
        std::shared_ptr<const DepthTexture2D> CaptureDepth() const;
        std::shared_ptr<const LinearTexture3D> CaptureVolume() const;
        std::shared_ptr<const BufferAllocation> CaptureStorage() const;
        std::array<std::uint32_t, 8> CreateViewDescriptor(bool disableSrgb, std::uint32_t firstMip = 0, std::uint32_t mipCount = 0, std::uint32_t firstLayer = 0, std::uint32_t layerCount = 0, FRHIViewDesc::EDimension viewDimension = FRHIViewDesc::EDimension::Unknown) const;
        std::vector<std::span<std::byte>> GetMipRanges(std::uint32_t mipLevel, std::uint32_t firstLayer = 0, std::uint32_t layerCount = 0) const;
        const BufferAllocation& GetStorage() const;
        std::size_t GetAllocationSize() const;

    private:
        friend class TextureInitializer;
        void finishInitialization();
        std::span<std::byte> getInitializationRange();
        const BufferAllocation& getAllocation() const;
        std::size_t getRowPitch() const;
        void uploadInitialData(std::span<const std::byte> data);
        FRHITextureSubresourceInitializer getSubresource(FRHITextureInitializer::FSubresourceIndex index);
        std::shared_ptr<Texture2D> storage;
        std::shared_ptr<DepthTexture2D> depthStorage;
        std::shared_ptr<LinearTexture3D> volumeStorage;
        bool initialized;
        std::vector<std::byte> volumeInitialData;
        FCriticalSection lockMutex;
        std::map<std::pair<std::uint32_t, std::uint32_t>, LockedData> lockedMips;
        std::optional<std::vector<std::byte>> mappedData;
    };

    UNREALPROSPERORHI_API TextureResource& ResolveTextureResource(FRHIResource* resource);
}

#endif
