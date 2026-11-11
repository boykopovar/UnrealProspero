#include "AnyUnreal/RHI/Command/ColorClear.h"
#include "AnyUnreal/RHI/Command/BufferCopy.h"
#include "AnyUnreal/RHI/Command/MemoryAcquire.h"
#include "prx/libSceAgc/DcbFlow/include/Memory.hpp"
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

        std::array<std::uint32_t, 4> pattern(TextureFormat format, const std::array<float, 4>& color)
        {
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
            if (format == TextureFormat::R32Float) word = std::bit_cast<std::uint32_t>(color[0]);
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
    }

    void WriteColorClear(CommandBuffer& commands, const LinearTexture2D& texture, const std::array<float, 4>& color)
    {
        const auto values = pattern(texture.GetFormat(), color);
        const auto& storage = texture.GetStorage();
        const GpuMemoryRange ranges[]{{storage.Data(), storage.Size()}};
        WriteMemoryAcquire(commands, ranges);
        sceAgcDcbWriteData(&commands, 5, 0, reinterpret_cast<std::uintptr_t>(storage.Data()), values.data(), static_cast<std::uint32_t>(values.size()), 0, 1);
        std::size_t written = sizeof(values);
        while (written < storage.Size())
        {
            const auto size = std::min(written, storage.Size() - written);
            WriteBufferCopy(commands, storage.GetRange(written, size).data(), storage.Data(), size);
            written += size;
        }
        WriteMemoryAcquire(commands, ranges);
    }
}
