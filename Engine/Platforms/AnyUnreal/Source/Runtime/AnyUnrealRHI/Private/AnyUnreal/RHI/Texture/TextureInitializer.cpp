#include "AnyUnreal/RHI/Texture/TextureInitializer.h"
#include "AnyUnreal/RHI/Texture/TextureResource.h"

namespace AnyUnreal::RHI
{
    class TextureInitializer final : public FRHITextureInitializer
    {
    public:
        TextureInitializer(FRHICommandListBase& commandList, TRefCountPtr<TextureResource> texture, bool initialize) : FRHITextureInitializer(commandList, texture.GetReference(), initialize ? texture->getAllocation().Data() : nullptr, initialize ? texture->getAllocation().Size() : 0, [texture, initialize](FRHICommandListBase&) -> FTextureRHIRef
        {
            if (initialize) texture->finishInitialization();
            return FTextureRHIRef(texture.GetReference());
        }, [texture](FSubresourceIndex index)
        {
            return texture->getSubresource(index);
        }) {}
    };

    FRHITextureInitializer CreateTextureInitializer(FRHICommandListBase& commandList, const FRHITextureCreateDesc& description, std::uint32_t addressHigh)
    {
        TRefCountPtr<TextureResource> texture = new TextureResource(description, addressHigh);
        return TextureInitializer(commandList, texture, description.InitAction == ERHITextureInitAction::Initializer);
    }
}
