#include "UnrealProspero/RHI/Command/ColorClear.h"
#include "UnrealProspero/RHI/Command/PatternFill.h"
#include "UnrealProspero/RHI/Texture/TextureClearValue.h"

namespace UnrealProspero::RHI
{
    void WriteColorClear(CommandBuffer& commands, const Texture2D& texture, const std::array<float, 4>& color, std::uint32_t mipLevel, std::uint32_t arrayLayer)
    {
        const auto values = EncodeTextureClear(texture.GetFormat(), color);
        const auto storage = texture.GetMipRange(mipLevel, arrayLayer);
        WritePatternFill(commands, storage.data(), storage.size(), values);
    }
}
