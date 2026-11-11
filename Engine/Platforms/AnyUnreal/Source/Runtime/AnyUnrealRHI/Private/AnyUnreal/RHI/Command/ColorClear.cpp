#include "AnyUnreal/RHI/Command/ColorClear.h"
#include "AnyUnreal/RHI/Command/PatternFill.h"
#include "AnyUnreal/RHI/Texture/ClearValue.h"

namespace AnyUnreal::RHI
{
    void WriteColorClear(CommandBuffer& commands, const LinearTexture2D& texture, const std::array<float, 4>& color)
    {
        const auto values = EncodeTextureClear(texture.GetFormat(), color);
        const auto& storage = texture.GetStorage();
        WritePatternFill(commands, storage.Data(), storage.Size(), values);
    }
}
