#include "AnyUnreal/RHI/Command/ColorClear.h"
#include "AnyUnreal/RHI/Command/PatternFill.h"
#include "AnyUnreal/RHI/Texture/ClearValue.h"
#include "AnyUnreal/RHI/Texture/LinearTexture3D.h"
#include "AnyUnreal/RHI/Command/CommandStorage.h"
#include "AnyUnreal/RHI/Command/Submission.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libSceAgc/libSceAgc.hpp"
#include "AnyUnreal/RHI/Memory/HalfFloat.h"
#include "AnyUnreal/RHI/Texture/HalfPixels.h"
#include <cmath>
#include <limits>
#include <algorithm>
#include <bit>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>

int main()
{
    using namespace AnyUnreal::RHI;
    for (std::uint32_t bits = 0; bits <= 65535; ++bits)
    {
        const auto exponent = (bits >> 10) & 31;
        const auto fraction = bits & 1023;
        const auto decoded = DecodeHalfFloat(static_cast<std::uint16_t>(bits));
        if (exponent == 31)
        {
            if (fraction == 0 ? !std::isinf(decoded) : !std::isnan(decoded)) throw std::runtime_error("Invalid half-float special value");
        }
        else
        {
            auto expected = exponent == 0 ? std::ldexp(static_cast<float>(fraction), -24) : std::ldexp(1.0f + static_cast<float>(fraction) / 1024.0f, static_cast<int>(exponent) - 15);
            if (bits & 0x8000) expected = -expected;
            if (std::bit_cast<std::uint32_t>(decoded) != std::bit_cast<std::uint32_t>(expected) || EncodeHalfFloat(decoded) != bits) throw std::runtime_error("Half-float conversion mismatch");
        }
    }
    const std::array roundingValues{1.00048828125f, 1.00146484375f, 65504.0f, 65520.0f, 0x1p-25f, 0x1.000002p-25f, -0x1p-25f};
    const std::array<std::uint16_t, 7> roundingBits{0x3c00, 0x3c02, 0x7bff, 0x7c00, 0, 1, 0x8000};
    for (std::size_t index = 0; index < roundingValues.size(); ++index)
    {
        if (EncodeHalfFloat(roundingValues[index]) != roundingBits[index]) throw std::runtime_error("Half-float rounding mismatch");
    }
    for (const auto format : {TextureFormat::R16Float, TextureFormat::Rg16Float, TextureFormat::Rgba16Float})
    {
        constexpr std::size_t width = 257;
        constexpr std::size_t height = 256;
        const auto channels = format == TextureFormat::R16Float ? 1u : format == TextureFormat::Rg16Float ? 2u : 4u;
        const auto pitch = width * channels * 2 + 6;
        std::vector<std::byte> bytes(pitch * height, std::byte{0xa5});
        for (std::size_t row = 0; row < height; ++row)
        for (std::size_t column = 0; column < width; ++column)
        for (std::size_t channel = 0; channel < channels; ++channel)
        {
            const auto value = static_cast<std::uint16_t>(row * width + column + channel);
            std::memcpy(bytes.data() + row * pitch + (column * channels + channel) * 2, &value, 2);
        }
        const auto expanded = ExpandHalfPixels(bytes, width, height, pitch, format);
        if (expanded.size() != width * height) throw std::runtime_error("Half-float expansion returned the wrong extent");
        for (std::size_t pixel = 0; pixel < expanded.size(); ++pixel)
        for (std::size_t channel = 0; channel < 4; ++channel)
        {
            const auto expected = channel < channels ? static_cast<std::uint16_t>(pixel + channel) : channel == 3 ? 0x3c00 : 0;
            if (expanded[pixel][channel] != expected) throw std::runtime_error("Half-float expansion changed bits or missing channels");
        }
        const auto reject = [&](auto action)
        {
            bool rejected = false;
            try { action(); }
            catch (const std::invalid_argument&) { rejected = true; }
            if (!rejected) throw std::runtime_error("Invalid half-float layout was accepted");
        };
        reject([&] { ExpandHalfPixels(bytes, width, height, 1, format); });
        reject([&] { ExpandHalfPixels(std::span(bytes).first(pitch * (height - 1)), width, height, pitch, format); });
        reject([&] { ExpandHalfPixels(bytes, 0, height, pitch, format); });
        reject([&] { ExpandHalfPixels(bytes, std::numeric_limits<std::size_t>::max(), height, pitch, format); });
        reject([&] { ExpandHalfPixels(bytes, width, height, pitch, TextureFormat::R32Float); });
    }
    AnyUnreal::Kernel::CheckResult(sceAgcInit(11), "sceAgcInit");
    const std::array colors{std::array{0.0f, 0.0f, 0.0f, 0.0f}, std::array{1.0f, 1.0f, 1.0f, 1.0f}, std::array{0.5f, 0.25f, 0.75f, 1.0f}, std::array{-1.0f, 2.0f, 0.0031308f, 0.5f}};
    const std::array linearBytes{std::array{0, 0, 0, 0}, std::array{255, 255, 255, 255}, std::array{128, 64, 191, 255}, std::array{0, 255, 1, 128}};
    const std::array srgbBytes{std::array{0, 0, 0, 0}, std::array{255, 255, 255, 255}, std::array{188, 137, 225, 255}, std::array{0, 255, 10, 128}};
    for (const auto format : {TextureFormat::R8Unorm, TextureFormat::Rg8Unorm, TextureFormat::Rgba8Unorm, TextureFormat::Rgba8Srgb, TextureFormat::Bgra8Unorm, TextureFormat::Bgra8Srgb, TextureFormat::R32Float, TextureFormat::Rgba32Float, TextureFormat::Rgba16Float, TextureFormat::R16Float, TextureFormat::Rg16Float})
    for (const auto dimensions : {std::array{1u, 1u}, std::array{67u, 9u}, std::array{129u, 37u}})
    for (std::size_t colorIndex = 0; colorIndex < colors.size(); ++colorIndex)
    {
        LinearTexture2D texture(dimensions[0], dimensions[1], format, 2);
        const auto& allocation = texture.GetStorage();
        if (allocation.AllocationSize() < allocation.Size() || allocation.AllocationSize() % 16384 != 0 || allocation.AllocationSize() - allocation.Size() >= 16384) throw std::runtime_error("Texture allocation size does not match kernel pages");
        const auto predictedSize = LinearTexture2D::CalculateDataSize(dimensions[0], dimensions[1], format);
        if (predictedSize != allocation.Size() || LinearTexture2D::CalculateRowPitch(dimensions[0], format) != texture.GetRowPitch() || BufferAllocation::CalculateAllocationSize(predictedSize) != allocation.AllocationSize()) throw std::runtime_error("Texture size query differs from its allocation");
        const auto bytes = allocation.GetRange(0, allocation.Size());
        std::fill(bytes.begin(), bytes.end(), std::byte{0xa5});
        std::array<std::byte, 16> pixel{};
        const auto pixelSize = texture.GetRowBytes() / texture.GetWidth();
        if (format == TextureFormat::R32Float || format == TextureFormat::Rgba32Float) std::memcpy(pixel.data(), colors[colorIndex].data(), pixelSize);
        else if (format == TextureFormat::Rgba16Float || format == TextureFormat::R16Float || format == TextureFormat::Rg16Float)
        {
            const std::array<std::array<std::uint16_t, 4>, 4> halfColors{{{0, 0, 0, 0}, {0x3c00, 0x3c00, 0x3c00, 0x3c00}, {0x3800, 0x3400, 0x3a00, 0x3c00}, {0xbc00, 0x4000, 0x1a69, 0x3800}}};
            std::memcpy(pixel.data(), halfColors[colorIndex].data(), pixelSize);
        }
        else
        {
            const bool srgb = format == TextureFormat::Rgba8Srgb || format == TextureFormat::Bgra8Srgb;
            const bool bgra = format == TextureFormat::Bgra8Unorm || format == TextureFormat::Bgra8Srgb;
            const auto values = srgb ? srgbBytes[colorIndex] : linearBytes[colorIndex];
            for (std::size_t index = 0; index < pixelSize; ++index) pixel[index] = static_cast<std::byte>(values[bgra && index < 3 ? 2 - index : index]);
        }
        BufferAllocation fenceStorage(8, 2);
        auto& fence = *std::construct_at(static_cast<std::uint64_t*>(fenceStorage.Data()), 0);
        CommandStorage storage(16384, 2);
        auto& commands = storage.Get();
        sceAgcDcbContextStateOp(&commands, 0);
        WriteColorClear(commands, texture, colors[colorIndex]);
        SubmitAndWait(commands, fence, 1, 10000000);
        for (std::size_t index = 0; index < bytes.size(); ++index)
        {
            if (bytes[index] != pixel[index % pixelSize]) throw std::runtime_error("Color clear pixel mismatch for format " + std::to_string(static_cast<int>(format)) + " and color " + std::to_string(colorIndex));
        }
    }
    for (const auto format : {TextureFormat::R8Unorm, TextureFormat::Rg8Unorm, TextureFormat::Rgba8Unorm, TextureFormat::Bgra8Unorm, TextureFormat::R32Float, TextureFormat::Rgba32Float, TextureFormat::Rgba16Float, TextureFormat::R16Float, TextureFormat::Rg16Float})
    {
        LinearTexture3D texture(7, 3, 5, format, 2);
        const std::array<std::uint32_t, 4> values{0x76543210u, 0xfedcba98u, 0x13579bdfu, 0x2468ace0u};
        const auto pixelSize = LinearTexture2D::CalculateRowBytes(1, format);
        const auto channelSize = format == TextureFormat::R32Float || format == TextureFormat::Rgba32Float ? 4u : IsHalfTextureFormat(format) ? 2u : 1u;
        std::array<std::byte, 16> pixel{};
        for (std::size_t byte = 0; byte < pixelSize; ++byte)
        {
            const auto channel = byte / channelSize;
            const auto source = format == TextureFormat::Bgra8Unorm && channel < 3 ? 2 - channel : channel;
            pixel[byte] = static_cast<std::byte>((values[source] >> ((byte % channelSize) * 8)) & 255u);
        }
        const auto& allocation = texture.GetStorage();
        const auto bytes = allocation.GetRange(0, allocation.Size());
        std::fill(bytes.begin(), bytes.end(), std::byte{0xa5});
        BufferAllocation fenceStorage(8, 2);
        auto& fence = *std::construct_at(static_cast<std::uint64_t*>(fenceStorage.Data()), 0);
        CommandStorage storage(16384, 2);
        auto& commands = storage.Get();
        sceAgcDcbContextStateOp(&commands, 0);
        WritePatternFill(commands, allocation.Data(), allocation.Size(), EncodeTextureClear(format, values));
        SubmitAndWait(commands, fence, 1, 10000000);
        for (std::size_t byte = 0; byte < bytes.size(); ++byte)
        {
            if (bytes[byte] != pixel[byte % pixelSize]) throw std::runtime_error("Integer volume clear changed component bits");
        }
    }
    std::cout << "Color clears verified for eleven formats, 132 float cases and nine integer volumes\n";
    return 0;
}
