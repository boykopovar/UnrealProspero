#include "AnyUnreal/RHI/Texture/TextureView.h"
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
            if (view.Dimension != FRHIViewDesc::EDimension::Texture2D) throw std::runtime_error("Texture view dimension is not implemented");
            if (view.Plane != ERHITexturePlane::Primary && view.Plane != ERHITexturePlane::PrimaryCompressed) throw std::runtime_error("Texture view plane is not implemented");
            if (view.Format != PF_Unknown && view.Format != resource.Format) throw std::runtime_error("Texture view format reinterpretation is not implemented");
            if (view.MipRange.First != 0 || view.MipRange.Num > 1 || view.ArrayRange.First != 0 || view.ArrayRange.Num > 1) throw std::out_of_range("Texture view exceeds the texture subresources");
            return description;
        }
    }

    TextureShaderResourceView::TextureShaderResourceView(TextureResource& texture, const FRHIViewDesc& description) : FRHIShaderResourceView(&texture, checkDescription(texture, description, false)), storage(texture.Capture()), descriptor(storage->CreateViewDescriptor(description.Texture.SRV.bDisableSRGB))
    {
        description.Texture.SRV.GetViewInfo(&texture);
    }

    TextureUnorderedAccessView::TextureUnorderedAccessView(TextureResource& texture, const FRHIViewDesc& description) : FRHIUnorderedAccessView(&texture, checkDescription(texture, description, true)), storage(texture.Capture()), descriptor(storage->CreateViewDescriptor(true))
    {
        description.Texture.UAV.GetViewInfo(&texture);
    }
}
