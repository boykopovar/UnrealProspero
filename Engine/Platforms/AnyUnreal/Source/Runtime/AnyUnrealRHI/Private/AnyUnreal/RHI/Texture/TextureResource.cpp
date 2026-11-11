#include "AnyUnreal/RHI/Texture/TextureResource.h"
#include "Containers/ResourceArray.h"
#include "RHITextureReference.h"
#include "Misc/ScopeLock.h"
#include <stdexcept>
#include <algorithm>

namespace AnyUnreal::RHI
{
    namespace
    {
        TextureFormat format(const FRHITextureDesc& description)
        {
            const bool srgb = EnumHasAnyFlags(description.Flags, ETextureCreateFlags::SRGB);
            switch (description.Format)
            {
            case PF_R8G8B8A8: return srgb ? TextureFormat::Rgba8Srgb : TextureFormat::Rgba8Unorm;
            case PF_B8G8R8A8: return srgb ? TextureFormat::Bgra8Srgb : TextureFormat::Bgra8Unorm;
            default: break;
            }
            if (srgb) throw std::runtime_error("sRGB is not implemented for this texture format");
            switch (description.Format)
            {
            case PF_G8:
            case PF_R8: return TextureFormat::R8Unorm;
            case PF_R8G8: return TextureFormat::Rg8Unorm;
            case PF_R32_FLOAT: return TextureFormat::R32Float;
            case PF_A32B32G32R32F: return TextureFormat::Rgba32Float;
            default: throw std::runtime_error("Texture pixel format is not implemented");
            }
        }

        void validateDescription(const FRHITextureDesc& description)
        {
            if (description.Dimension != ETextureDimension::Texture2D || description.Depth != 1 || description.ArraySize != 1 || description.NumMips != 1 || description.NumSamples != 1) throw std::runtime_error("Texture creation currently requires a single-mip non-multisampled 2D texture");
            if (description.Extent.X <= 0 || description.Extent.Y <= 0 || description.Extent.X > 16384 || description.Extent.Y > 16384) throw std::invalid_argument("Texture extent exceeds hardware limits");
            if (description.GPUMask.GetNative() != 1 || !description.AliasableFormats.IsEmpty() || description.ExtData != 0 || description.UAVFormat != PF_Unknown) throw std::runtime_error("Texture creation requires an unimplemented allocation or format mode");
            using Flags = ETextureCreateFlags;
            constexpr auto supported = Flags::ShaderResource | Flags::UAV | Flags::RenderTargetable | Flags::SRGB | Flags::NoTiling | Flags::DisableDCC | Flags::FastVRAM | Flags::HideInVisualizeTexture | Flags::MultiGPUGraphIgnore | Flags::CPUReadback;
            if (EnumHasAnyFlags(description.Flags, ~supported)) throw std::runtime_error("Texture flags require unimplemented behavior");
        }

        const FRHITextureCreateDesc& validate(const FRHITextureCreateDesc& description)
        {
            validateDescription(description);
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

    std::size_t TextureResource::CalculateAllocationSize(const FRHITextureDesc& description)
    {
        validateDescription(description);
        return BufferAllocation::CalculateAllocationSize(LinearTexture2D::CalculateDataSize(description.Extent.X, description.Extent.Y, format(description)));
    }

    TextureResource::TextureResource(const FRHITextureCreateDesc& description, std::uint32_t addressHigh) : FRHITexture(validate(description)), storage(std::make_shared<LinearTexture2D>(description.Extent.X, description.Extent.Y, format(description), addressHigh)), initialized(description.InitAction != ERHITextureInitAction::Initializer)
    {
        if (description.InitAction == ERHITextureInitAction::BulkData)
        {
            const auto size = description.BulkData->GetResourceBulkDataSize();
            if (size != storage->GetRowBytes() * description.Extent.Y) throw std::invalid_argument("Texture bulk data size does not match its extent");
            storage->Upload({static_cast<const std::byte*>(description.BulkData->GetResourceBulkData()), size}, storage->GetRowBytes());
            description.BulkData->Discard();
        }
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

    std::size_t TextureResource::GetAllocationSize() const { return storage->GetStorage().AllocationSize(); }

    std::shared_ptr<const LinearTexture2D> TextureResource::Capture() const
    {
        if (!initialized) throw std::runtime_error("Texture initialization has not been finalized");
        return storage;
    }

    FRHITextureSubresourceInitializer TextureResource::getSubresource(FRHITextureInitializer::FSubresourceIndex index)
    {
        if (initialized) throw std::runtime_error("Texture is not awaiting initialization");
        if (index.FaceIndex != 0 || index.ArrayIndex != 0 || index.MipIndex != 0) throw std::out_of_range("Texture subresource does not exist");
        return {storage->GetStorage().Data(), storage->GetStorage().Size(), storage->GetRowPitch()};
    }

    void TextureResource::finishInitialization()
    {
        if (initialized) throw std::runtime_error("Texture is already initialized");
        initialized = true;
    }
}
