#include "AnyUnreal/RHI/Buffer/ClearValue.h"
#include "AnyUnreal/RHI/Memory/HalfFloat.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace AnyUnreal::RHI
{
    std::array<std::uint32_t, 4> EncodeBufferClear(BufferFormat format, const std::array<std::uint32_t, 4>& values)
    {
        std::array<std::uint32_t, 4> result{};
        switch (format)
        {
        case BufferFormat::Untyped:
        case BufferFormat::Uint1:
        case BufferFormat::Float1: result[0] = values[0]; break;
        case BufferFormat::Float2: std::copy_n(values.begin(), 2, result.begin()); break;
        case BufferFormat::Float3: std::copy_n(values.begin(), 3, result.begin()); break;
        case BufferFormat::Float4: result = values; break;
        case BufferFormat::Byte4Snorm:
        case BufferFormat::Byte4Uint:
        case BufferFormat::Byte4Unorm:
        case BufferFormat::Bgra8Unorm:
            for (std::size_t channel = 0; channel < 4; ++channel)
            {
                const auto source = format == BufferFormat::Bgra8Unorm && channel % 2 == 0 ? 2 - channel : channel;
                result[0] |= (values[source] & 255u) << (channel * 8);
            }
            break;
        case BufferFormat::Short2Sint:
        case BufferFormat::Short4Sint:
        case BufferFormat::Short2Snorm:
        case BufferFormat::Short4Snorm:
        case BufferFormat::Half2:
        case BufferFormat::Half4:
        case BufferFormat::Short2Uint:
        case BufferFormat::Short4Uint:
        case BufferFormat::Short2Unorm:
        case BufferFormat::Short4Unorm:
            for (std::size_t channel = 0; channel < GetBufferElementSize(format) / 2; ++channel) result[channel / 2] |= (values[channel] & 65535u) << ((channel % 2) * 16);
            break;
        case BufferFormat::Rgb10A2Unorm: result[0] = (values[0] & 1023u) | ((values[1] & 1023u) << 10) | ((values[2] & 1023u) << 20) | ((values[3] & 3u) << 30); break;
        default: throw std::invalid_argument("Unsupported buffer clear format");
        }
        return result;
    }

    std::array<std::uint32_t, 4> EncodeBufferClear(BufferFormat format, const std::array<float, 4>& values)
    {
        std::array<std::uint32_t, 4> encoded{};
        if (format == BufferFormat::Untyped || format == BufferFormat::Float1 || format == BufferFormat::Float2 || format == BufferFormat::Float3 || format == BufferFormat::Float4)
        {
            for (std::size_t channel = 0; channel < 4; ++channel) encoded[channel] = std::bit_cast<std::uint32_t>(values[channel]);
            return EncodeBufferClear(format, encoded);
        }
        if (format == BufferFormat::Half2 || format == BufferFormat::Half4)
        {
            for (std::size_t channel = 0; channel < GetBufferElementSize(format) / 2; ++channel) encoded[channel] = EncodeHalfFloat(values[channel]);
            return EncodeBufferClear(format, encoded);
        }
        const bool signedNormalized = format == BufferFormat::Byte4Snorm || format == BufferFormat::Short2Snorm || format == BufferFormat::Short4Snorm;
        const bool byteNormalized = format == BufferFormat::Byte4Snorm || format == BufferFormat::Byte4Unorm || format == BufferFormat::Bgra8Unorm;
        const bool shortNormalized = format == BufferFormat::Short2Snorm || format == BufferFormat::Short4Snorm || format == BufferFormat::Short2Unorm || format == BufferFormat::Short4Unorm;
        if (!byteNormalized && !shortNormalized && format != BufferFormat::Rgb10A2Unorm) throw std::invalid_argument("Floating point buffer clear requires a floating point or normalized format");
        const auto channels = shortNormalized ? GetBufferElementSize(format) / 2 : 4;
        for (std::size_t channel = 0; channel < channels; ++channel)
        {
            if (std::isnan(values[channel])) throw std::invalid_argument("Normalized buffer clear cannot encode NaN");
            const auto bits = byteNormalized ? 8 : shortNormalized ? 16 : channel == 3 ? 2 : 10;
            const auto maximum = (1u << (bits - (signedNormalized ? 1 : 0))) - 1;
            const auto value = std::clamp(values[channel], signedNormalized ? -1.0f : 0.0f, 1.0f);
            encoded[channel] = static_cast<std::uint32_t>(static_cast<std::int32_t>(std::round(value * maximum)));
        }
        return EncodeBufferClear(format, encoded);
    }
}
