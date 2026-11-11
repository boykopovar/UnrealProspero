#include "AnyUnreal/RHI/Command/ColorClear.h"
#include "AnyUnreal/RHI/Command/CommandStorage.h"
#include "AnyUnreal/RHI/Command/Submission.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libSceAgc/libSceAgc.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>

int main()
{
    using namespace AnyUnreal::RHI;
    AnyUnreal::Kernel::CheckResult(sceAgcInit(11), "sceAgcInit");
    const std::array colors{std::array{0.0f, 0.0f, 0.0f, 0.0f}, std::array{1.0f, 1.0f, 1.0f, 1.0f}, std::array{0.5f, 0.25f, 0.75f, 1.0f}, std::array{-1.0f, 2.0f, 0.0031308f, 0.5f}};
    const std::array linearBytes{std::array{0, 0, 0, 0}, std::array{255, 255, 255, 255}, std::array{128, 64, 191, 255}, std::array{0, 255, 1, 128}};
    const std::array srgbBytes{std::array{0, 0, 0, 0}, std::array{255, 255, 255, 255}, std::array{188, 137, 225, 255}, std::array{0, 255, 10, 128}};
    for (const auto format : {TextureFormat::R8Unorm, TextureFormat::Rg8Unorm, TextureFormat::Rgba8Unorm, TextureFormat::Rgba8Srgb, TextureFormat::Bgra8Unorm, TextureFormat::Bgra8Srgb, TextureFormat::R32Float, TextureFormat::Rgba32Float})
    for (const auto dimensions : {std::array{1u, 1u}, std::array{67u, 9u}, std::array{129u, 37u}})
    for (std::size_t colorIndex = 0; colorIndex < colors.size(); ++colorIndex)
    {
        LinearTexture2D texture(dimensions[0], dimensions[1], format, 2);
        const auto bytes = texture.GetStorage().GetRange(0, texture.GetStorage().Size());
        std::fill(bytes.begin(), bytes.end(), std::byte{0xa5});
        std::array<std::byte, 16> pixel{};
        const auto pixelSize = texture.GetRowBytes() / texture.GetWidth();
        if (format == TextureFormat::R32Float || format == TextureFormat::Rgba32Float) std::memcpy(pixel.data(), colors[colorIndex].data(), pixelSize);
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
    std::cout << "Color clears verified for eight formats and 96 cases\n";
    return 0;
}
