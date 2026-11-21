#include "UnrealProspero/RHI/Texture/TextureResource.h"
#include "UnrealProspero/RHI/Texture/PixelFormat.h"
#include "UnrealProspero/RHI/Device/DeviceLimits.h"
#include "Containers/ResourceArray.h"
#include "RHITextureReference.h"
#include "Misc/ScopeLock.h"
#include <stdexcept>
#include <algorithm>
#include <string>

namespace UnrealProspero::RHI
{
    namespace
    {
        AllocationUsage allocationUsage(const FRHITextureDesc& description)
        {
            return EnumHasAnyFlags(description.Flags, ETextureCreateFlags::Streamable) && !EnumHasAnyFlags(description.Flags, ETextureCreateFlags::ForceIntoNonStreamingMemoryTracking) ? AllocationUsage::StreamingTexture : AllocationUsage::Texture;
        }

        bool isDepthTarget(const FRHITextureDesc& description)
        {
            return description.Format == PF_DepthStencil || (description.Format == PF_ShadowDepth && EnumHasAnyFlags(description.Flags, ETextureCreateFlags::DepthStencilTargetable));
        }

        void validateDescription(const FRHITextureDesc& description)
        {
            const bool volume = description.Dimension == ETextureDimension::Texture3D;
            const bool cube = description.IsTextureCube();
            const bool array = description.Dimension == ETextureDimension::Texture2DArray || description.Dimension == ETextureDimension::TextureCubeArray;
            if ((!volume && !array && !cube && description.Dimension != ETextureDimension::Texture2D) || (!volume && description.Depth != 1) || (!array && description.ArraySize != 1) || description.NumSamples != 1) throw std::runtime_error("Texture creation requires a non-multisampled 2D, cube or 3D texture");
            if (array && (description.ArraySize == 0 || description.ArraySize > (cube ? 8192 / 6 : 8192))) throw std::out_of_range("Texture array exceeds the descriptor layer range");
            if (cube && description.Extent.X != description.Extent.Y) throw std::invalid_argument("Cube texture faces must be square");
            if (volume && (description.Depth == 0 || description.Depth > DeviceLimits::Texture3D || description.Extent.X > static_cast<int32>(DeviceLimits::Texture3D) || description.Extent.Y > static_cast<int32>(DeviceLimits::Texture3D))) throw std::invalid_argument("Volume texture extent exceeds hardware limits");
            if (description.Extent.X <= 0 || description.Extent.Y <= 0 || description.Extent.X > static_cast<int32>(DeviceLimits::Texture2D) || description.Extent.Y > static_cast<int32>(DeviceLimits::Texture2D)) throw std::invalid_argument("Texture extent exceeds hardware limits");
            if (description.GPUMask.GetNative() != 1 || description.ExtData != 0 || description.UAVFormat != PF_Unknown) throw std::runtime_error("Texture allocation mode is not implemented: gpu=" + std::to_string(description.GPUMask.GetNative()) + ", aliases=" + std::to_string(description.AliasableFormats.Num()) + ", ext=" + std::to_string(description.ExtData) + ", uav=" + std::to_string(static_cast<int>(description.UAVFormat)) + ", format=" + std::to_string(static_cast<int>(description.Format)));
            for (const auto format : description.AliasableFormats)
            {
                if (format != description.Format && !(description.Format == PF_DepthStencil && format == PF_X24_G8)) throw std::runtime_error("Texture alias format is not implemented: " + std::to_string(static_cast<int>(format)) + ", base=" + std::to_string(static_cast<int>(description.Format)));
            }
            using Flags = ETextureCreateFlags;
            if (description.Format == PF_ShadowDepth && !isDepthTarget(description) && EnumHasAnyFlags(description.Flags, Flags::RenderTargetable | Flags::UAV | Flags::Presentable | Flags::SRGB)) throw std::invalid_argument("Shader-only depth texture flags are incompatible with its format");
            if ((description.Format == PF_R32_UINT || description.Format == PF_R32_SINT) && EnumHasAnyFlags(description.Flags, Flags::RenderTargetable)) throw std::runtime_error("Integer render targets are not implemented");
            if (isDepthTarget(description) || EnumHasAnyFlags(description.Flags, Flags::Presentable))
            {
                if (description.NumMips != 1) throw std::runtime_error("Depth and presentable mip chains are not implemented");
            }
            else if (volume) LinearTexture3D::CalculateDataSize(description.Extent.X, description.Extent.Y, description.Depth, ResolveTextureFormat(description.Format, EnumHasAnyFlags(description.Flags, Flags::SRGB)), description.NumMips);
            else Texture2D::CalculateMipLayout(description.Extent.X, description.Extent.Y, ResolveTextureFormat(description.Format, EnumHasAnyFlags(description.Flags, Flags::SRGB)), description.NumMips);
            if ((array || cube) && EnumHasAnyFlags(description.Flags, Flags::DepthStencilTargetable | Flags::Presentable)) throw std::runtime_error("Array depth and presentable targets are not implemented");
            if (volume && EnumHasAnyFlags(description.Flags, Flags::DepthStencilTargetable | Flags::Presentable)) throw std::runtime_error("Volume depth and presentable targets are not implemented");
            if (isDepthTarget(description))
            {
                if (description.Dimension != ETextureDimension::Texture2D) throw std::runtime_error("Tiled depth storage requires a 2D texture");
                constexpr auto depthFlags = Flags::InputAttachmentRead | Flags::TargetArraySlicesIndependently | Flags::ForceIntoNonStreamingMemoryTracking | Flags::NoFastClearFinalize | Flags::ShaderResource | Flags::DepthStencilTargetable | Flags::NoFastClear | Flags::DisableDCC | Flags::FastVRAM | Flags::HideInVisualizeTexture | Flags::MultiGPUGraphIgnore;
                if (EnumHasAnyFlags(description.Flags, ~depthFlags)) throw std::runtime_error("Depth texture flags require unimplemented behavior: " + std::to_string(static_cast<std::uint64_t>(description.Flags & ~depthFlags)));
                return;
            }
            const auto formatInfo = DescribeTextureFormat(ResolveTextureFormat(description.Format, EnumHasAnyFlags(description.Flags, Flags::SRGB)));
            if (description.Format == PF_A8 && EnumHasAnyFlags(description.Flags, Flags::UAV | Flags::RenderTargetable)) throw std::invalid_argument("Alpha-only textures do not support render target or unordered access usage");
            if (EnumHasAnyFlags(description.Flags, Flags::AtomicCompatible) && description.Format != PF_R32_UINT && description.Format != PF_R32_SINT) throw std::runtime_error("Texture atomics require a supported 32-bit integer format: " + std::to_string(static_cast<int>(description.Format)));
            if (formatInfo.BlockWidth > 1 && EnumHasAnyFlags(description.Flags, Flags::UAV | Flags::RenderTargetable | Flags::Presentable)) throw std::invalid_argument("Compressed textures do not support render target or unordered access usage");
            constexpr auto supported = Flags::AtomicCompatible | Flags::Tiling3D | Flags::ReduceMemoryWithTilingMode | Flags::InputAttachmentRead | Flags::NoFastClearFinalize | Flags::TargetArraySlicesIndependently | Flags::OfflineProcessed | Flags::Streamable | Flags::ForceIntoNonStreamingMemoryTracking | Flags::ShaderResource | Flags::UAV | Flags::RenderTargetable | Flags::SRGB | Flags::NoTiling | Flags::NoFastClear | Flags::DisableDCC | Flags::FastVRAM | Flags::HideInVisualizeTexture | Flags::MultiGPUGraphIgnore | Flags::CPUReadback | Flags::Presentable;
            if (EnumHasAnyFlags(description.Flags, Flags::Presentable) && description.Format != PF_B8G8R8A8 && description.Format != PF_R8G8B8A8 && description.Format != PF_A2B10G10R10) throw std::invalid_argument("Presentable texture format is not supported");
            if (EnumHasAnyFlags(description.Flags, ~supported)) throw std::runtime_error("Texture flags require unimplemented behavior: " + std::to_string(static_cast<std::uint64_t>(description.Flags & ~supported)));
        }

        const FRHITextureCreateDesc& validate(const FRHITextureCreateDesc& description)
        {
            validateDescription(description);
            if (isDepthTarget(description) && description.InitAction != ERHITextureInitAction::Default) throw std::runtime_error("CPU initialization of tiled depth textures is not implemented");
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
        return isDepthTarget(description) || EnumHasAnyFlags(description.Flags, ETextureCreateFlags::Presentable) ? 65536 : Kernel::DirectAllocation::PageSize;
    }

    std::size_t TextureResource::CalculateAllocationSize(const FRHITextureDesc& description, std::uint32_t firstMip)
    {
        validateDescription(description);
        if (firstMip >= description.NumMips) throw std::out_of_range("Texture size selects an unavailable mip");
        if (isDepthTarget(description)) return DepthTexture2D::CalculateDataSize(description.Extent.X, description.Extent.Y, description.Format == PF_DepthStencil);
        if (description.Dimension == ETextureDimension::Texture3D) return BufferAllocation::CalculateAllocationSize(LinearTexture3D::CalculateDataSize(description.Extent.X, description.Extent.Y, description.Depth, ResolveTextureFormat(description.Format, EnumHasAnyFlags(description.Flags, ETextureCreateFlags::SRGB)), description.NumMips, firstMip));
        return BufferAllocation::CalculateAllocationSize(Texture2D::CalculateDataSize(description.Extent.X, description.Extent.Y, ResolveTextureFormat(description.Format, EnumHasAnyFlags(description.Flags, ETextureCreateFlags::SRGB)), description.NumMips, firstMip) * description.ArraySize * (description.IsTextureCube() ? 6 : 1));
    }

    TextureResource::TextureResource(const FRHITextureCreateDesc& description, std::uint32_t addressHigh) : FRHITexture(validate(description)), initialized(description.InitAction != ERHITextureInitAction::Initializer)
    {
        if (isDepthTarget(description)) depthStorage = std::make_shared<DepthTexture2D>(description.Extent.X, description.Extent.Y, addressHigh, description.Format == PF_DepthStencil);
        else if (description.Dimension == ETextureDimension::Texture3D) volumeStorage = std::make_shared<LinearTexture3D>(description.Extent.X, description.Extent.Y, description.Depth, ResolveTextureFormat(description.Format, EnumHasAnyFlags(description.Flags, ETextureCreateFlags::SRGB)), addressHigh, description.NumMips, allocationUsage(description));
        else storage = std::make_shared<Texture2D>(description.Extent.X, description.Extent.Y, ResolveTextureFormat(description.Format, EnumHasAnyFlags(description.Flags, ETextureCreateFlags::SRGB)), addressHigh, CalculateAlignment(description), description.NumMips, allocationUsage(description), description.ArraySize * (description.IsTextureCube() ? 6 : 1));
        if (volumeStorage && description.InitAction == ERHITextureInitAction::Initializer)
        {
            std::size_t size = 0;
            for (std::uint32_t level = 0; level < volumeStorage->GetMipCount(); ++level)
            {
                const auto& mip = volumeStorage->GetMipLayout(level);
                size += mip.RowBytes * mip.RowCount * mip.Depth;
            }
            volumeInitialData.resize(size);
        }
        if (description.InitAction == ERHITextureInitAction::BulkData)
        {
            const auto size = description.BulkData->GetResourceBulkDataSize();
            uploadInitialData({static_cast<const std::byte*>(description.BulkData->GetResourceBulkData()), size});
            description.BulkData->Discard();
        }
    }

    TextureResource::TextureResource(const FRHITextureCreateDesc& description, std::uint32_t addressHigh, std::span<void* const> initialMipData) : TextureResource(description, addressHigh)
    {
        if (description.InitAction != ERHITextureInitAction::Default || description.BulkData) throw std::invalid_argument("Initial texture data conflicts with its initialization action");
        if (!storage || storage->GetArrayLayers() != 1) throw std::invalid_argument("Mip initialization requires a single 2D color texture");
        if (initialMipData.empty() || initialMipData.size() > storage->GetMipCount()) throw std::invalid_argument("Initial texture mip count exceeds its allocation");
        for (const auto* data : initialMipData) if (!data) throw std::invalid_argument("Initial texture mip data is null");
        for (std::uint32_t level = 0; level < initialMipData.size(); ++level)
        {
            const auto& mip = storage->GetMipLayout(level);
            storage->Upload({static_cast<const std::byte*>(initialMipData[level]), mip.RowBytes * mip.RowCount}, mip.RowBytes, level);
        }
    }

    TextureResource::TextureResource(const FRHITextureCreateDesc& description, std::shared_ptr<Texture2D> externalStorage) : FRHITexture(validate(description)), storage(std::move(externalStorage)), initialized(true)
    {
        if (description.Dimension != ETextureDimension::Texture2D) throw std::invalid_argument("External 2D storage requires a 2D texture description");
        if (description.InitAction != ERHITextureInitAction::Default || description.BulkData) throw std::invalid_argument("External texture storage cannot be initialized again");
        if (!storage || storage->GetArrayLayers() != 1 || storage->GetMipCount() != description.NumMips || storage->GetWidth() != static_cast<uint32>(description.Extent.X) || storage->GetHeight() != static_cast<uint32>(description.Extent.Y) || storage->GetFormat() != ResolveTextureFormat(description.Format, EnumHasAnyFlags(description.Flags, ETextureCreateFlags::SRGB))) throw std::invalid_argument("External texture storage does not match its description");
        if (reinterpret_cast<std::uintptr_t>(storage->GetStorage().Data()) % CalculateAlignment(description) != 0) throw std::invalid_argument("External texture storage does not meet its alignment");
    }

    void* TextureResource::Lock(EResourceLockMode mode, std::uint32_t mipLevel, std::size_t& size, std::size_t& pitch, std::uint32_t arrayLayer)
    {
        FScopeLock lock(&lockMutex);
        if (lockedMips.contains({arrayLayer, mipLevel}) || mappedData) throw std::runtime_error("Texture is already locked or mapped");
        if (mode != RLM_ReadOnly && mode != RLM_WriteOnly) throw std::invalid_argument("Invalid texture lock mode");
        const auto texture = Capture();
        const auto& mip = texture->GetMipLayout(mipLevel);
        const auto range = texture->GetMipRange(mipLevel, arrayLayer);
        LockedData next{mode, mipLevel, arrayLayer, mip.RowPitch, std::vector<std::byte>(range.size())};
        if (mode == RLM_ReadOnly)
        {
            texture->CopyRowsTo(next.Data, mip.RowPitch, mipLevel, arrayLayer);
        }
        auto& entry = lockedMips.emplace(std::pair{arrayLayer, mipLevel}, std::move(next)).first->second;
        size = entry.Data.size();
        pitch = entry.Pitch;
        return entry.Data.data();
    }

    std::uint32_t TextureResource::GetArrayLayer(std::uint32_t arrayIndex, std::uint32_t faceIndex) const
    {
        const std::uint32_t faceCount = GetDesc().IsTextureCube() ? 6 : 1;
        if (arrayIndex >= GetDesc().ArraySize || faceIndex >= faceCount) throw std::out_of_range("Texture face or array index does not exist");
        return arrayIndex * faceCount + faceIndex;
    }

    void* TextureResource::MapReadback()
    {
        FScopeLock lock(&lockMutex);
        if (!lockedMips.empty() || mappedData) throw std::runtime_error("Texture is already locked or mapped");
        if (GetDesc().Dimension != ETextureDimension::Texture2D) throw std::invalid_argument("Mapped readback requires a 2D texture");
        if (!EnumHasAnyFlags(GetDesc().Flags, ETextureCreateFlags::CPUReadback)) throw std::invalid_argument("Texture was not created for CPU readback");
        const auto texture = Capture();
        std::vector<std::byte> data(texture->GetRowBytes() * texture->GetMipLayout(0).RowCount);
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

    TextureResource::LockedData TextureResource::Unlock(std::uint32_t mipLevel, std::uint32_t arrayLayer)
    {
        FScopeLock lock(&lockMutex);
        const auto found = lockedMips.find({arrayLayer, mipLevel});
        if (found == lockedMips.end()) throw std::runtime_error("Texture mip is not locked");
        auto result = std::move(found->second);
        lockedMips.erase(found);
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

    std::array<std::uint32_t, 8> TextureResource::CreateViewDescriptor(bool disableSrgb, std::uint32_t firstMip, std::uint32_t mipCount, std::uint32_t firstLayer, std::uint32_t layerCount, FRHIViewDesc::EDimension viewDimension, ERHITexturePlane plane) const
    {
        if (!initialized) throw std::runtime_error("Texture initialization has not been finalized");
        using Dimension = FRHIViewDesc::EDimension;
        if (viewDimension == Dimension::Unknown)
        {
            switch (GetDesc().Dimension)
            {
            case ETextureDimension::Texture2D: viewDimension = Dimension::Texture2D; break;
            case ETextureDimension::Texture2DArray: viewDimension = Dimension::Texture2DArray; break;
            case ETextureDimension::TextureCube: viewDimension = Dimension::TextureCube; break;
            case ETextureDimension::TextureCubeArray: viewDimension = Dimension::TextureCubeArray; break;
            case ETextureDimension::Texture3D: viewDimension = Dimension::Texture3D; break;
            default: throw std::invalid_argument("Texture dimension is not supported");
            }
        }
        const bool cubeView = viewDimension == Dimension::TextureCube || viewDimension == Dimension::TextureCubeArray;
        const auto availableLayers = storage && !cubeView ? storage->GetArrayLayers() : GetDesc().ArraySize;
        if (firstLayer >= availableLayers || layerCount > availableLayers - firstLayer) throw std::out_of_range("Texture view exceeds its array layers");
        if (cubeView)
        {
            if (!GetDesc().IsTextureCube()) throw std::invalid_argument("Cube view requires cube storage");
            return storage->CreateCubeViewDescriptor(disableSrgb, firstLayer, layerCount == 0 ? availableLayers - firstLayer : layerCount, firstMip, mipCount);
        }
        if (viewDimension == Dimension::Texture2DArray)
        {
            if (!storage) throw std::invalid_argument("Array view requires linear color storage");
            return storage->CreateArrayViewDescriptor(disableSrgb, firstLayer, layerCount == 0 ? availableLayers - firstLayer : layerCount, firstMip, mipCount);
        }
        if (viewDimension == Dimension::Texture2D && (layerCount == 0 ? GetDesc().ArraySize : layerCount) != 1) throw std::invalid_argument("2D view must select exactly one layer");
        if (depthStorage && (firstMip != 0 || mipCount > 1)) throw std::out_of_range("Texture view exceeds the available mip range");
        if (depthStorage) return depthStorage->GetDescriptor(plane == ERHITexturePlane::Stencil);
        if (volumeStorage) return volumeStorage->CreateViewDescriptor(disableSrgb, firstMip, mipCount);
        return storage->CreateViewDescriptor(disableSrgb, firstMip, mipCount, firstLayer);
    }

    std::vector<std::span<std::byte>> TextureResource::GetMipRanges(std::uint32_t mipLevel, std::uint32_t firstLayer, std::uint32_t layerCount) const
    {
        if (!initialized) throw std::runtime_error("Texture initialization has not been finalized");
        const auto availableLayers = storage ? storage->GetArrayLayers() : 1;
        if (firstLayer >= availableLayers || layerCount > availableLayers - firstLayer) throw std::out_of_range("Texture ranges exceed the array layers");
        if (storage)
        {
            if (layerCount == 0) layerCount = storage->GetArrayLayers() - firstLayer;
            std::vector<std::span<std::byte>> ranges;
            ranges.reserve(layerCount);
            for (std::uint32_t layer = 0; layer < layerCount; ++layer) ranges.push_back(storage->GetMipRange(mipLevel, firstLayer + layer));
            return ranges;
        }
        if (volumeStorage)
        {
            const auto& mip = volumeStorage->GetMipLayout(mipLevel);
            std::vector<std::span<std::byte>> ranges;
            ranges.reserve(mip.Depth);
            for (std::uint32_t slice = 0; slice < mip.Depth; ++slice) ranges.push_back(volumeStorage->GetSliceRange(mipLevel, slice));
            return ranges;
        }
        if (mipLevel != 0) throw std::out_of_range("Texture mip level does not exist");
        const auto& allocation = getAllocation();
        return {allocation.GetRange(0, allocation.Size())};
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
        if (storage)
        {
            std::size_t required = 0;
            for (std::uint32_t level = 0; level < storage->GetMipCount(); ++level)
            {
                const auto& mip = storage->GetMipLayout(level);
                required += mip.RowBytes * mip.RowCount;
            }
            if (data.size() != required * storage->GetArrayLayers()) throw std::invalid_argument("Initial texture data does not match its mip chain");
            std::size_t offset = 0;
            for (std::uint32_t level = 0; level < storage->GetMipCount(); ++level)
            for (std::uint32_t layer = 0; layer < storage->GetArrayLayers(); ++layer)
            {
                const auto& mip = storage->GetMipLayout(level);
                const auto size = mip.RowBytes * mip.RowCount;
                storage->Upload(data.subspan(offset, size), mip.RowBytes, level, layer);
                offset += size;
            }
            return;
        }
        if (!volumeStorage) throw std::runtime_error("CPU initialization of tiled depth textures is not implemented");
        std::size_t required = 0;
        for (std::uint32_t level = 0; level < volumeStorage->GetMipCount(); ++level)
        {
            const auto& mip = volumeStorage->GetMipLayout(level);
            required += mip.RowBytes * mip.RowCount * mip.Depth;
        }
        if (data.size() != required) throw std::invalid_argument("Initial volume data does not match its mip chain");
        std::size_t offset = 0;
        for (std::uint32_t level = 0; level < volumeStorage->GetMipCount(); ++level)
        {
            const auto& mip = volumeStorage->GetMipLayout(level);
            const auto sliceBytes = mip.RowBytes * mip.RowCount;
            const auto size = sliceBytes * mip.Depth;
            volumeStorage->Upload(data.subspan(offset, size), mip.RowBytes, sliceBytes, level);
            offset += size;
        }
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

    std::shared_ptr<const Texture2D> TextureResource::Capture() const
    {
        if (!initialized) throw std::runtime_error("Texture initialization has not been finalized");
        if (!storage) throw std::runtime_error("This operation requires a linear color texture");
        return storage;
    }

    FRHITextureSubresourceInitializer TextureResource::getSubresource(FRHITextureInitializer::FSubresourceIndex index)
    {
        if (initialized) throw std::runtime_error("Texture is not awaiting initialization");
        if (index.FaceIndex < 0 || index.ArrayIndex < 0 || index.ArrayIndex >= GetDesc().ArraySize || index.MipIndex < 0 || index.MipIndex >= GetDesc().NumMips) throw std::out_of_range("Texture subresource does not exist");
        const auto layer = GetArrayLayer(index.ArrayIndex, index.FaceIndex);
        if (storage)
        {
            const auto range = storage->GetMipRange(index.MipIndex, layer);
            return {range.data(), range.size(), storage->GetMipLayout(index.MipIndex).RowPitch};
        }
        if (volumeStorage)
        {
            std::size_t offset = 0;
            for (int32 level = 0; level < index.MipIndex; ++level)
            {
                const auto& mip = volumeStorage->GetMipLayout(level);
                offset += mip.RowBytes * mip.RowCount * mip.Depth;
            }
            const auto& mip = volumeStorage->GetMipLayout(index.MipIndex);
            return {volumeInitialData.data() + offset, mip.RowBytes * mip.RowCount * mip.Depth, mip.RowBytes};
        }
        const auto& allocation = getAllocation();
        return {allocation.Data(), allocation.Size(), getRowPitch()};
    }

    std::span<std::byte> TextureResource::getInitializationRange()
    {
        if (initialized) throw std::runtime_error("Texture is not awaiting initialization");
        if (volumeStorage) return volumeInitialData;
        const auto& allocation = getAllocation();
        return allocation.GetRange(0, allocation.Size());
    }

    void TextureResource::finishInitialization()
    {
        if (initialized) throw std::runtime_error("Texture is already initialized");
        if (volumeStorage)
        {
            uploadInitialData(volumeInitialData);
            std::vector<std::byte>().swap(volumeInitialData);
        }
        initialized = true;
    }
}
