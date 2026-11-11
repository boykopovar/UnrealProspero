#include "AnyUnreal/RHI/Texture/TextureResource.h"
#include "AnyUnreal/RHI/Texture/PixelFormat.h"
#include "Containers/ResourceArray.h"
#include "RHITextureReference.h"
#include "Misc/ScopeLock.h"
#include <stdexcept>
#include <algorithm>

namespace AnyUnreal::RHI
{
    namespace
    {
        void validateDescription(const FRHITextureDesc& description)
        {
            const bool volume = description.Dimension == ETextureDimension::Texture3D;
            if ((!volume && description.Dimension != ETextureDimension::Texture2D) || (!volume && description.Depth != 1) || description.ArraySize != 1 || description.NumMips != 1 || description.NumSamples != 1) throw std::runtime_error("Texture creation requires a single-mip non-multisampled 2D or 3D texture");
            if (volume && (description.Depth == 0 || description.Depth > 2048 || description.Extent.X > 2048 || description.Extent.Y > 2048)) throw std::invalid_argument("Volume texture extent exceeds hardware limits");
            if (description.Extent.X <= 0 || description.Extent.Y <= 0 || description.Extent.X > 16384 || description.Extent.Y > 16384) throw std::invalid_argument("Texture extent exceeds hardware limits");
            if (description.GPUMask.GetNative() != 1 || !description.AliasableFormats.IsEmpty() || description.ExtData != 0 || description.UAVFormat != PF_Unknown) throw std::runtime_error("Texture creation requires an unimplemented allocation or format mode");
            using Flags = ETextureCreateFlags;
            if (volume && (description.Format == PF_ShadowDepth || EnumHasAnyFlags(description.Flags, Flags::RenderTargetable | Flags::DepthStencilTargetable | Flags::Presentable))) throw std::runtime_error("Volume render targets are not implemented");
            if (description.Format == PF_ShadowDepth)
            {
                constexpr auto depthFlags = Flags::ShaderResource | Flags::DepthStencilTargetable | Flags::DisableDCC | Flags::FastVRAM | Flags::HideInVisualizeTexture | Flags::MultiGPUGraphIgnore;
                if (!EnumHasAnyFlags(description.Flags, Flags::DepthStencilTargetable)) throw std::invalid_argument("Depth texture requires depth target usage");
                if (EnumHasAnyFlags(description.Flags, ~depthFlags)) throw std::runtime_error("Depth texture flags require unimplemented behavior");
                return;
            }
            constexpr auto supported = Flags::ShaderResource | Flags::UAV | Flags::RenderTargetable | Flags::SRGB | Flags::NoTiling | Flags::DisableDCC | Flags::FastVRAM | Flags::HideInVisualizeTexture | Flags::MultiGPUGraphIgnore | Flags::CPUReadback | Flags::Presentable;
            if (EnumHasAnyFlags(description.Flags, Flags::Presentable) && description.Format != PF_B8G8R8A8 && description.Format != PF_R8G8B8A8) throw std::invalid_argument("Presentable texture format is not supported");
            if (EnumHasAnyFlags(description.Flags, ~supported)) throw std::runtime_error("Texture flags require unimplemented behavior");
        }

        const FRHITextureCreateDesc& validate(const FRHITextureCreateDesc& description)
        {
            validateDescription(description);
            if (description.Format == PF_ShadowDepth && description.InitAction != ERHITextureInitAction::Default) throw std::runtime_error("CPU initialization of tiled depth textures is not implemented");
            switch (description.InitAction)
            {
            case ERHITextureInitAction::Default:
            case ERHITextureInitAction::Initializer:
                if (description.BulkData) throw std::invalid_argument("Texture bulk data requires the bulk-data initialization action");
                break;
            case ERHITextureInitAction::BulkData:
                if (!description.BulkData || !description.BulkData->GetResourceBulkData()) throw std::invalid_argument("Texture bulk data is missing");
                break;
            default: throw std::invalid_argument("Unknown texture initialization action");
            }
            return description;
        }
    }

    TextureResource& ResolveTextureResource(FRHIResource* resource)
    {
        if (!resource) throw std::invalid_argument("Texture resource is null");
        if (resource->GetType() == RRT_TextureReference) resource = static_cast<FRHITextureReference*>(resource)->GetReferencedTexture();
        if (!resource || resource->GetType() != RRT_Texture) throw std::invalid_argument("Texture reference does not resolve to a texture resource");
        return *static_cast<TextureResource*>(resource);
    }

    std::size_t TextureResource::CalculateAlignment(const FRHITextureDesc& description)
    {
        validateDescription(description);
        return description.Format == PF_ShadowDepth || EnumHasAnyFlags(description.Flags, ETextureCreateFlags::Presentable) ? 65536 : Kernel::DirectAllocation::PageSize;
    }

    std::size_t TextureResource::CalculateAllocationSize(const FRHITextureDesc& description)
    {
        validateDescription(description);
        if (description.Format == PF_ShadowDepth) return DepthTexture2D::CalculateDataSize(description.Extent.X, description.Extent.Y);
        if (description.Dimension == ETextureDimension::Texture3D) return BufferAllocation::CalculateAllocationSize(LinearTexture3D::CalculateDataSize(description.Extent.X, description.Extent.Y, description.Depth, ResolveTextureFormat(description.Format, EnumHasAnyFlags(description.Flags, ETextureCreateFlags::SRGB))));
        return BufferAllocation::CalculateAllocationSize(LinearTexture2D::CalculateDataSize(description.Extent.X, description.Extent.Y, ResolveTextureFormat(description.Format, EnumHasAnyFlags(description.Flags, ETextureCreateFlags::SRGB))));
    }

    TextureResource::TextureResource(const FRHITextureCreateDesc& description, std::uint32_t addressHigh) : FRHITexture(validate(description)), initialized(description.InitAction != ERHITextureInitAction::Initializer)
    {
        if (description.Format == PF_ShadowDepth) depthStorage = std::make_shared<DepthTexture2D>(description.Extent.X, description.Extent.Y, addressHigh);
        else if (description.Dimension == ETextureDimension::Texture3D) volumeStorage = std::make_shared<LinearTexture3D>(description.Extent.X, description.Extent.Y, description.Depth, ResolveTextureFormat(description.Format, EnumHasAnyFlags(description.Flags, ETextureCreateFlags::SRGB)), addressHigh);
        else storage = std::make_shared<LinearTexture2D>(description.Extent.X, description.Extent.Y, ResolveTextureFormat(description.Format, EnumHasAnyFlags(description.Flags, ETextureCreateFlags::SRGB)), addressHigh, CalculateAlignment(description));
        if (description.InitAction == ERHITextureInitAction::BulkData)
        {
            const auto size = description.BulkData->GetResourceBulkDataSize();
            uploadInitialData({static_cast<const std::byte*>(description.BulkData->GetResourceBulkData()), size});
            description.BulkData->Discard();
        }
    }

    TextureResource::TextureResource(const FRHITextureCreateDesc& description, std::uint32_t addressHigh, const void* initialData) : TextureResource(description, addressHigh)
    {
        if (description.InitAction != ERHITextureInitAction::Default || description.BulkData) throw std::invalid_argument("Initial texture data conflicts with its initialization action");
        if (!initialData) throw std::invalid_argument("Initial texture data is null");
        const auto size = LinearTexture2D::CalculateRowBytes(description.Extent.X, ResolveTextureFormat(description.Format, EnumHasAnyFlags(description.Flags, ETextureCreateFlags::SRGB))) * description.Extent.Y * description.Depth;
        uploadInitialData({static_cast<const std::byte*>(initialData), size});
    }

    TextureResource::TextureResource(const FRHITextureCreateDesc& description, std::shared_ptr<LinearTexture2D> externalStorage) : FRHITexture(validate(description)), storage(std::move(externalStorage)), initialized(true)
    {
        if (description.Dimension != ETextureDimension::Texture2D) throw std::invalid_argument("External 2D storage requires a 2D texture description");
        if (description.InitAction != ERHITextureInitAction::Default || description.BulkData) throw std::invalid_argument("External texture storage cannot be initialized again");
        if (!storage || storage->GetWidth() != static_cast<uint32>(description.Extent.X) || storage->GetHeight() != static_cast<uint32>(description.Extent.Y) || storage->GetFormat() != ResolveTextureFormat(description.Format, EnumHasAnyFlags(description.Flags, ETextureCreateFlags::SRGB))) throw std::invalid_argument("External texture storage does not match its description");
        if (reinterpret_cast<std::uintptr_t>(storage->GetStorage().Data()) % CalculateAlignment(description) != 0) throw std::invalid_argument("External texture storage does not meet its alignment");
    }

    void* TextureResource::Lock(EResourceLockMode mode, std::size_t& size, std::size_t& pitch)
    {
        FScopeLock lock(&lockMutex);
        if (lockedData || mappedData) throw std::runtime_error("Texture is already locked or mapped");
        if (mode != RLM_ReadOnly && mode != RLM_WriteOnly) throw std::invalid_argument("Invalid texture lock mode");
        const auto texture = Capture();
        const auto& allocation = texture->GetStorage();
        LockedData next{mode, texture->GetRowPitch(), std::vector<std::byte>(allocation.Size())};
        if (mode == RLM_ReadOnly)
        {
            const auto source = allocation.GetRange(0, allocation.Size());
            std::copy(source.begin(), source.end(), next.Data.begin());
        }
        lockedData = std::move(next);
        size = lockedData->Data.size();
        pitch = lockedData->Pitch;
        return lockedData->Data.data();
    }

    void* TextureResource::MapReadback()
    {
        FScopeLock lock(&lockMutex);
        if (lockedData || mappedData) throw std::runtime_error("Texture is already locked or mapped");
        if (!EnumHasAnyFlags(GetDesc().Flags, ETextureCreateFlags::CPUReadback)) throw std::invalid_argument("Texture was not created for CPU readback");
        const auto texture = Capture();
        std::vector<std::byte> data(texture->GetRowBytes() * texture->GetHeight());
        texture->CopyRowsTo(data, texture->GetRowBytes());
        mappedData = std::move(data);
        return mappedData->data();
    }

    void TextureResource::UnmapReadback()
    {
        FScopeLock lock(&lockMutex);
        if (!mappedData) throw std::runtime_error("Texture is not mapped for readback");
        mappedData.reset();
    }

    TextureResource::LockedData TextureResource::Unlock()
    {
        FScopeLock lock(&lockMutex);
        if (!lockedData) throw std::runtime_error("Texture is not locked");
        auto result = std::move(*lockedData);
        lockedData.reset();
        return result;
    }

    std::size_t TextureResource::GetAllocationSize() const { return getAllocation().AllocationSize(); }

    std::shared_ptr<const BufferAllocation> TextureResource::CaptureStorage() const
    {
        const auto& allocation = GetStorage();
        if (depthStorage) return {depthStorage, &allocation};
        if (volumeStorage) return {volumeStorage, &allocation};
        return {storage, &allocation};
    }

    std::array<std::uint32_t, 8> TextureResource::CreateViewDescriptor(bool disableSrgb) const
    {
        if (!initialized) throw std::runtime_error("Texture initialization has not been finalized");
        if (depthStorage) return depthStorage->GetDescriptor();
        if (volumeStorage) return volumeStorage->CreateViewDescriptor(disableSrgb);
        return storage->CreateViewDescriptor(disableSrgb);
    }

    const BufferAllocation& TextureResource::GetStorage() const
    {
        if (!initialized) throw std::runtime_error("Texture initialization has not been finalized");
        return getAllocation();
    }

    const BufferAllocation& TextureResource::getAllocation() const
    {
        if (volumeStorage) return volumeStorage->GetStorage();
        return depthStorage ? depthStorage->GetStorage() : storage->GetStorage();
    }

    std::size_t TextureResource::getRowPitch() const
    {
        if (depthStorage) throw std::runtime_error("Linear initialization of a depth texture is not implemented");
        return volumeStorage ? volumeStorage->GetRowPitch() : storage->GetRowPitch();
    }

    void TextureResource::uploadInitialData(std::span<const std::byte> data)
    {
        const auto& description = GetDesc();
        const auto rowBytes = LinearTexture2D::CalculateRowBytes(description.Extent.X, ResolveTextureFormat(description.Format, EnumHasAnyFlags(description.Flags, ETextureCreateFlags::SRGB)));
        const auto sliceBytes = rowBytes * description.Extent.Y;
        if (data.size() != sliceBytes * description.Depth) throw std::invalid_argument("Initial texture data size does not match its extent");
        if (volumeStorage) volumeStorage->Upload(data, rowBytes, sliceBytes);
        else if (storage) storage->Upload(data, rowBytes);
        else throw std::runtime_error("CPU initialization of tiled depth textures is not implemented");
    }

    std::shared_ptr<const LinearTexture3D> TextureResource::CaptureVolume() const
    {
        if (!initialized) throw std::runtime_error("Texture initialization has not been finalized");
        if (!volumeStorage) throw std::invalid_argument("Texture does not contain volume storage");
        return volumeStorage;
    }

    std::shared_ptr<const DepthTexture2D> TextureResource::CaptureDepth() const
    {
        if (!initialized) throw std::runtime_error("Texture initialization has not been finalized");
        if (!depthStorage) throw std::invalid_argument("Texture does not contain a depth plane");
        return depthStorage;
    }

    std::shared_ptr<const LinearTexture2D> TextureResource::Capture() const
    {
        if (!initialized) throw std::runtime_error("Texture initialization has not been finalized");
        if (!storage) throw std::runtime_error("This operation requires a linear color texture");
        return storage;
    }

    FRHITextureSubresourceInitializer TextureResource::getSubresource(FRHITextureInitializer::FSubresourceIndex index)
    {
        if (initialized) throw std::runtime_error("Texture is not awaiting initialization");
        if (index.FaceIndex != 0 || index.ArrayIndex != 0 || index.MipIndex != 0) throw std::out_of_range("Texture subresource does not exist");
        const auto& allocation = getAllocation();
        return {allocation.Data(), allocation.Size(), getRowPitch()};
    }

    void TextureResource::finishInitialization()
    {
        if (initialized) throw std::runtime_error("Texture is already initialized");
        initialized = true;
    }
}
