#include "AnyUnreal/RHI/Texture/TextureClearValue.h"
#include "AnyUnreal/RHI/Memory/HalfFloat.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace AnyUnreal::RHI
{
    namespace
    {
        std::uint32_t normalized(float value, bool srgb)
        {
            if (std::isnan(value)) throw std::invalid_argument("Normalized color clear cannot encode NaN");
            value = std::clamp(value, 0.0f, 1.0f);
            if (srgb) value = value <= 0.0031308f ? value * 12.92f : 1.055f * std::pow(value, 1.0f / 2.4f) - 0.055f;
            return static_cast<std::uint32_t>(std::floor(value * 255.0f + 0.5f));
        }

    }

    std::array<std::uint32_t, 4> EncodeTextureClear(TextureFormat format, const std::array<float, 4>& color)
    {
        if (format == TextureFormat::R32Uint || format == TextureFormat::R32Sint) throw std::invalid_argument("Integer texture clear requires integer values");
        std::array<std::uint32_t, 4> result{};
        if (format == TextureFormat::Rgba32Float)
        {
            for (std::size_t index = 0; index < result.size(); ++index) result[index] = std::bit_cast<std::uint32_t>(color[index]);
            return result;
        }
        if (format == TextureFormat::Rgba16Float)
        {
            result[0] = EncodeHalfFloat(color[0]) | (static_cast<std::uint32_t>(EncodeHalfFloat(color[1])) << 16);
            result[1] = EncodeHalfFloat(color[2]) | (static_cast<std::uint32_t>(EncodeHalfFloat(color[3])) << 16);
            result[2] = result[0];
            result[3] = result[1];
            return result;
        }
        std::uint32_t word;
        if (format == TextureFormat::R16Float) word = static_cast<std::uint32_t>(EncodeHalfFloat(color[0])) * 0x00010001u;
        else if (format == TextureFormat::Rg16Float) word = EncodeHalfFloat(color[0]) | (static_cast<std::uint32_t>(EncodeHalfFloat(color[1])) << 16);
        else if (format == TextureFormat::R32Float) word = std::bit_cast<std::uint32_t>(color[0]);
        else
        {
            const bool srgb = format == TextureFormat::Rgba8Srgb || format == TextureFormat::Bgra8Srgb;
            const auto red = normalized(color[0], srgb);
            if (format == TextureFormat::R8Unorm) word = red * 0x01010101u;
            else
            {
                const auto green = normalized(color[1], srgb);
                if (format == TextureFormat::Rg8Unorm) word = (red | (green << 8)) * 0x00010001u;
                else
                {
                    const auto blue = normalized(color[2], srgb);
                    const auto alpha = normalized(color[3], false);
                    switch (format)
                    {
                    case TextureFormat::Rgba8Unorm:
                    case TextureFormat::Rgba8Srgb: word = red | (green << 8) | (blue << 16) | (alpha << 24); break;
                    case TextureFormat::Bgra8Unorm:
                    case TextureFormat::Bgra8Srgb: word = blue | (green << 8) | (red << 16) | (alpha << 24); break;
                    default: throw std::invalid_argument("Unsupported color clear format");
                    }
                }
            }
        }
        result.fill(word);
        return result;
    }

    std::array<std::uint32_t, 4> EncodeTextureClear(TextureFormat format, const std::array<std::uint32_t, 4>& values)
    {
        std::array<std::uint32_t, 4> result{};
        const auto pixelBytes = LinearTexture2D::CalculateRowBytes(1, format);
        if (format == TextureFormat::Rgba32Float) return values;
        if (format == TextureFormat::R32Float || format == TextureFormat::R32Uint || format == TextureFormat::R32Sint) result.fill(values[0]);
        else if (format == TextureFormat::Rgba16Float || format == TextureFormat::Rg16Float || format == TextureFormat::R16Float)
        {
            const auto channels = pixelBytes / 2;
            for (std::size_t channel = 0; channel < 8; ++channel) result[channel / 2] |= (values[channel % channels] & 65535u) << ((channel % 2) * 16);
        }
        else
        {
            const bool bgra = format == TextureFormat::Bgra8Unorm || format == TextureFormat::Bgra8Srgb;
            for (std::size_t byte = 0; byte < 16; ++byte)
            {
                const auto channel = byte % pixelBytes;
                const auto source = bgra && channel < 3 ? 2 - channel : channel;
                result[byte / 4] |= (values[source] & 255u) << ((byte % 4) * 8);
            }
        }
        return result;
    }
}
