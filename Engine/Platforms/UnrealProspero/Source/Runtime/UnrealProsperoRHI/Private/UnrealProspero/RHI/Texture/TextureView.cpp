#include "UnrealProspero/RHI/Texture/TextureView.h"
#include "UnrealProspero/RHI/Texture/PixelFormat.h"
#include <stdexcept>

namespace UnrealProspero::RHI
{
    namespace
    {
        const FRHIViewDesc& checkDescription(const TextureResource& texture, const FRHIViewDesc& description, bool unorderedAccess)
        {
            const auto expectedType = unorderedAccess ? FRHIViewDesc::EViewType::TextureUAV : FRHIViewDesc::EViewType::TextureSRV;
            if (description.Common.ViewType != expectedType) throw std::invalid_argument("Texture view description does not match the requested access");
            const FRHIViewDesc::FTexture& view = unorderedAccess ? static_cast<const FRHIViewDesc::FTexture&>(description.Texture.UAV) : description.Texture.SRV;
            const auto& resource = texture.GetDesc();
            const bool accessible = unorderedAccess ? EnumHasAnyFlags(resource.Flags, ETextureCreateFlags::UAV) : texture.SupportsShaderAccess();
            if (!accessible) throw std::invalid_argument("Texture does not allow the requested view access");
            if (unorderedAccess && view.MipRange.Num != 1) throw std::invalid_argument("Texture UAV must select exactly one mip level");
            const auto expectedDimension = resource.Dimension == ETextureDimension::Texture3D ? FRHIViewDesc::EDimension::Texture3D : resource.Dimension == ETextureDimension::Texture2DArray ? FRHIViewDesc::EDimension::Texture2DArray : FRHIViewDesc::EDimension::Texture2D;
            const bool cubeView = view.Dimension == FRHIViewDesc::EDimension::TextureCube || view.Dimension == FRHIViewDesc::EDimension::TextureCubeArray;
            const bool cubeResource = resource.IsTextureCube();
            if (cubeResource ? (!cubeView && view.Dimension != FRHIViewDesc::EDimension::Texture2DArray && view.Dimension != FRHIViewDesc::EDimension::Texture2D) : view.Dimension != expectedDimension && !((resource.Dimension == ETextureDimension::Texture2DArray && view.Dimension == FRHIViewDesc::EDimension::Texture2D) || (resource.Dimension == ETextureDimension::Texture2D && view.Dimension == FRHIViewDesc::EDimension::Texture2DArray))) throw std::invalid_argument("Texture view dimension does not match its resource");
            const bool depth = resource.Format == PF_ShadowDepth || resource.Format == PF_DepthStencil;
            const bool stencil = resource.Format == PF_DepthStencil && (view.Plane == ERHITexturePlane::Stencil || view.Format == PF_X24_G8);
            if (view.Plane != ERHITexturePlane::Primary && view.Plane != ERHITexturePlane::PrimaryCompressed && !(depth && view.Plane == ERHITexturePlane::Depth) && !(stencil && view.Plane == ERHITexturePlane::Stencil)) throw std::runtime_error("Texture view plane is not implemented");
            if (view.Format != PF_Unknown && view.Format != resource.Format && !(depth && view.Format == PF_R32_FLOAT) && !(stencil && view.Format == PF_X24_G8)) throw std::runtime_error("Texture view format reinterpretation is not implemented");
            const auto availableLayers = cubeResource && !cubeView ? resource.ArraySize * 6 : resource.ArraySize;
            if (view.Dimension == FRHIViewDesc::EDimension::Texture2D && (view.ArrayRange.Num == 0 ? resource.ArraySize : view.ArrayRange.Num) != 1) throw std::invalid_argument("2D view must select exactly one layer");
            if (view.Dimension == FRHIViewDesc::EDimension::TextureCube && (view.ArrayRange.Num == 0 ? availableLayers : view.ArrayRange.Num) != 1) throw std::invalid_argument("Cube view must select exactly one cube");
            if ((view.MipRange.Num == 0 && view.MipRange.First != 0) || view.MipRange.First >= resource.NumMips || view.MipRange.Num > resource.NumMips - view.MipRange.First || view.ArrayRange.First >= availableLayers || view.ArrayRange.Num > availableLayers - view.ArrayRange.First || (view.ArrayRange.Num == 0 && view.ArrayRange.First != 0)) throw std::out_of_range("Texture view exceeds the texture subresources");
            return description;
        }
    }

    TextureShaderResourceView::TextureShaderResourceView(TextureResource& texture, const FRHIViewDesc& description) : FRHIShaderResourceView(&texture, checkDescription(texture, description, false)), storage(texture.CaptureStorage()), descriptor(texture.CreateViewDescriptor(description.Texture.SRV.bDisableSRGB, description.Texture.SRV.MipRange.First, description.Texture.SRV.MipRange.Num, description.Texture.SRV.ArrayRange.First, description.Texture.SRV.ArrayRange.Num, description.Texture.SRV.Dimension, description.Texture.SRV.Format == PF_X24_G8 ? ERHITexturePlane::Stencil : description.Texture.SRV.Plane))
    {
        description.Texture.SRV.GetViewInfo(&texture);
    }

    TextureUnorderedAccessView::TextureUnorderedAccessView(TextureResource& texture, const FRHIViewDesc& description) : FRHIUnorderedAccessView(&texture, checkDescription(texture, description, true)), format(ResolveTextureFormat(texture.GetDesc().Format, false)), storage(texture.CaptureStorage())
    {
        const auto info = description.Texture.UAV.GetViewInfo(&texture);
        auto dimension = info.Dimension;
        std::uint32_t firstLayer = info.ArrayRange.First;
        std::uint32_t layerCount = info.ArrayRange.Num;
        if (dimension == FRHIViewDesc::EDimension::TextureCube || dimension == FRHIViewDesc::EDimension::TextureCubeArray)
        {
            dimension = FRHIViewDesc::EDimension::Texture2DArray;
            firstLayer *= 6;
            layerCount *= 6;
        }
        descriptor = texture.CreateViewDescriptor(true, info.MipLevel, 1, firstLayer, layerCount, dimension);
        ranges = texture.GetMipRanges(info.MipLevel, firstLayer, layerCount);
    }
}
