#include "AnyUnreal/RHI/Command/TextureCopy.h"
#include "AnyUnreal/RHI/Command/BufferFill.h"
#include "AnyUnreal/RHI/Command/CommandStorage.h"
#include "AnyUnreal/RHI/Command/Submission.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libSceAgc/libSceAgc.hpp"
#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

int main()
{
    using namespace AnyUnreal::RHI;
    AnyUnreal::Kernel::CheckResult(sceAgcInit(11), "sceAgcInit");
    for (const auto format : {TextureFormat::R8Unorm, TextureFormat::Rg8Unorm, TextureFormat::Rgba8Unorm, TextureFormat::Rgba8Srgb, TextureFormat::Bgra8Unorm, TextureFormat::Bgra8Srgb, TextureFormat::R32Float, TextureFormat::Rgba32Float})
    for (const unsigned mode : {0u, 1u, 2u, 3u, 4u})
    {
        LinearTexture2D source(67, 9, format, 2);
        auto destinationFormat = format;
        if (mode == 4 && format == TextureFormat::Rgba8Srgb) destinationFormat = TextureFormat::Rgba8Unorm;
        if (mode == 4 && format == TextureFormat::Bgra8Srgb) destinationFormat = TextureFormat::Bgra8Unorm;
        LinearTexture2D other(91, 13, destinationFormat, 2);
        const auto& destination = mode == 2 ? source : other;
        const auto sourceBytes = source.GetStorage().GetRange(0, source.GetStorage().Size());
        const auto destinationBytes = destination.GetStorage().GetRange(0, destination.GetStorage().Size());
        std::fill(destinationBytes.begin(), destinationBytes.end(), std::byte{0xa5});
        for (std::size_t index = 0; index < sourceBytes.size(); ++index) sourceBytes[index] = static_cast<std::byte>((index * 37 + index / source.GetRowPitch()) % 251);
        std::vector original(sourceBytes.begin(), sourceBytes.end());
        if (mode == 3)
        {
            for (std::size_t index = 0; index < original.size(); ++index) original[index] = static_cast<std::byte>((0x76543210u >> ((index % 4) * 8)) & 255);
        }
        std::vector expected(destinationBytes.begin(), destinationBytes.end());
        const auto region = mode == 0 ? TextureCopyRegion{0, 0, 0, 0, 67, 9} : TextureCopyRegion{1, 1, 40, 6, 5, 3};
        const auto bytesPerPixel = source.GetRowBytes() / source.GetWidth();
        for (std::uint32_t row = 0; row < region.Height; ++row)
        {
            const auto from = (region.SourceY + row) * source.GetRowPitch() + region.SourceX * bytesPerPixel;
            const auto to = (region.DestinationY + row) * destination.GetRowPitch() + region.DestinationX * bytesPerPixel;
            std::copy_n(original.begin() + from, region.Width * bytesPerPixel, expected.begin() + to);
        }
        BufferAllocation fenceStorage(8, 2);
        auto& fence = *std::construct_at(static_cast<std::uint64_t*>(fenceStorage.Data()), 0);
        CommandStorage storage(16384, 2);
        auto& commands = storage.Get();
        sceAgcDcbContextStateOp(&commands, 0);
        if (mode == 3) WriteBufferFill(commands, source.GetStorage().Data(), source.GetStorage().Size(), 0x76543210u);
        WriteTextureCopy(commands, destination, source, region);
        SubmitAndWait(commands, fence, 1, 10000000);
        if (!std::equal(expected.begin(), expected.end(), destinationBytes.begin())) throw std::runtime_error("Texture copy changed incorrect bytes or row padding");
        if (mode != 2 && !std::equal(original.begin(), original.end(), sourceBytes.begin())) throw std::runtime_error("Texture copy modified its source");
    }
    LinearTexture2D texture(17, 7, TextureFormat::Rgba8Unorm, 2);
    LinearTexture2D incompatible(17, 7, TextureFormat::R32Float, 2);
    CommandStorage storage(4096, 2);
    auto& commands = storage.Get();
    const auto reject = [&](const LinearTexture2D& destination, const TextureCopyRegion& region)
    {
        const auto before = commands.cursor_up;
        bool rejected = false;
        try { WriteTextureCopy(commands, destination, texture, region); }
        catch (const std::invalid_argument&) { rejected = true; }
        catch (const std::out_of_range&) { rejected = true; }
        if (!rejected || commands.cursor_up != before) throw std::runtime_error("Invalid texture copy was recorded");
    };
    reject(texture, {0, 0, 1, 1, 3, 3});
    reject(texture, {0, 0, 0, 0, 0, 1});
    reject(texture, {17, 0, 0, 0, 1, 1});
    reject(texture, {0, 0, 17, 0, 1, 1});
    reject(texture, {0, 0, 0, 6, 1, 2});
    reject(incompatible, {0, 0, 0, 0, 1, 1});
    std::cout << "Texture copies and invalid regions verified\n";
    return 0;
}
