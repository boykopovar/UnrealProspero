#include "AnyUnreal/RHI/Texture/TextureView.h"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    namespace
    {
        const FRHIViewDesc& checkDescription(const TextureResource& texture, const FRHIViewDesc& description)
        {
            if (description.Common.ViewType != FRHIViewDesc::EViewType::TextureSRV) throw std::invalid_argument("Texture shader resource view requires a texture SRV description");
            const auto& view = description.Texture.SRV;
            const auto& resource = texture.GetDesc();
            if (!EnumHasAnyFlags(resource.Flags, ETextureCreateFlags::ShaderResource)) throw std::invalid_argument("Texture does not allow shader resource access");
            if (view.Dimension != FRHIViewDesc::EDimension::Texture2D) throw std::runtime_error("Texture shader resource view dimension is not implemented");
            if (view.Plane != ERHITexturePlane::Primary && view.Plane != ERHITexturePlane::PrimaryCompressed) throw std::runtime_error("Texture shader resource view plane is not implemented");
            if (view.Format != PF_Unknown && view.Format != resource.Format) throw std::runtime_error("Texture shader resource view format reinterpretation is not implemented");
            if (view.MipRange.First != 0 || view.MipRange.Num > 1 || view.ArrayRange.First != 0 || view.ArrayRange.Num > 1) throw std::out_of_range("Texture shader resource view exceeds the texture subresources");
            return description;
        }
    }

    TextureShaderResourceView::TextureShaderResourceView(TextureResource& texture, const FRHIViewDesc& description) : FRHIShaderResourceView(&texture, checkDescription(texture, description)), storage(texture.Capture()), descriptor(storage->CreateViewDescriptor(description.Texture.SRV.bDisableSRGB))
    {
        description.Texture.SRV.GetViewInfo(&texture);
    }
}
