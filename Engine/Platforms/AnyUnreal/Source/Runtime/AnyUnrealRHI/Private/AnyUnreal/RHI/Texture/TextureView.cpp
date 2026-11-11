#include "AnyUnreal/RHI/Texture/TextureView.h"
#include "AnyUnreal/RHI/Texture/PixelFormat.h"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    namespace
    {
        const FRHIViewDesc& checkDescription(const TextureResource& texture, const FRHIViewDesc& description, bool unorderedAccess)
        {
            const auto expectedType = unorderedAccess ? FRHIViewDesc::EViewType::TextureUAV : FRHIViewDesc::EViewType::TextureSRV;
            if (description.Common.ViewType != expectedType) throw std::invalid_argument("Texture view description does not match the requested access");
            const FRHIViewDesc::FTexture& view = unorderedAccess ? static_cast<const FRHIViewDesc::FTexture&>(description.Texture.UAV) : description.Texture.SRV;
            const auto& resource = texture.GetDesc();
            const auto requiredFlag = unorderedAccess ? ETextureCreateFlags::UAV : ETextureCreateFlags::ShaderResource;
            if (!EnumHasAnyFlags(resource.Flags, requiredFlag)) throw std::invalid_argument("Texture does not allow the requested view access");
            if (unorderedAccess && view.MipRange.Num != 1) throw std::invalid_argument("Texture UAV must select exactly one mip level");
            const auto expectedDimension = resource.Dimension == ETextureDimension::Texture3D ? FRHIViewDesc::EDimension::Texture3D : FRHIViewDesc::EDimension::Texture2D;
            if (view.Dimension != expectedDimension) throw std::invalid_argument("Texture view dimension does not match its resource");
            const bool depth = resource.Format == PF_ShadowDepth;
            if (view.Plane != ERHITexturePlane::Primary && view.Plane != ERHITexturePlane::PrimaryCompressed && !(depth && view.Plane == ERHITexturePlane::Depth)) throw std::runtime_error("Texture view plane is not implemented");
            if (view.Format != PF_Unknown && view.Format != resource.Format && !(depth && view.Format == PF_R32_FLOAT)) throw std::runtime_error("Texture view format reinterpretation is not implemented");
            if (view.MipRange.First != 0 || view.MipRange.Num > 1 || view.ArrayRange.First != 0 || view.ArrayRange.Num > 1) throw std::out_of_range("Texture view exceeds the texture subresources");
            return description;
        }
    }

    TextureShaderResourceView::TextureShaderResourceView(TextureResource& texture, const FRHIViewDesc& description) : FRHIShaderResourceView(&texture, checkDescription(texture, description, false)), storage(texture.CaptureStorage()), descriptor(texture.CreateViewDescriptor(description.Texture.SRV.bDisableSRGB))
    {
        description.Texture.SRV.GetViewInfo(&texture);
    }

    TextureUnorderedAccessView::TextureUnorderedAccessView(TextureResource& texture, const FRHIViewDesc& description) : FRHIUnorderedAccessView(&texture, checkDescription(texture, description, true)), format(ResolveTextureFormat(texture.GetDesc().Format, false)), storage(texture.CaptureStorage()), descriptor(texture.CreateViewDescriptor(true))
    {
        description.Texture.UAV.GetViewInfo(&texture);
    }
}
