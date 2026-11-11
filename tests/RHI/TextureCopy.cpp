#include "AnyUnreal/RHI/Command/TextureCopy.h"
#include "AnyUnreal/RHI/Memory/TextureUpload.h"
#include "AnyUnreal/RHI/Memory/VolumeUpload.h"
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
#include <limits>

void CheckVolumeCopies()
{
    using namespace AnyUnreal::RHI;
    for (const auto format : {TextureFormat::R8Unorm, TextureFormat::Rg8Unorm, TextureFormat::Rgba8Unorm, TextureFormat::Rgba8Srgb, TextureFormat::Bgra8Unorm, TextureFormat::Bgra8Srgb, TextureFormat::R32Float, TextureFormat::Rgba32Float, TextureFormat::Rgba16Float})
    for (unsigned mode = 0; mode < 4; ++mode)
    {
        LinearTexture3D source(13, 7, 5, format, 2);
        auto targetFormat = format;
        if (mode == 3 && format == TextureFormat::Rgba8Srgb) targetFormat = TextureFormat::Rgba8Unorm;
        if (mode == 3 && format == TextureFormat::Bgra8Srgb) targetFormat = TextureFormat::Bgra8Unorm;
        LinearTexture3D other(17, 9, 7, targetFormat, 2);
        const auto& destination = mode == 2 ? source : other;
        const auto sourceBytes = source.GetStorage().GetRange(0, source.GetStorage().Size());
        const auto targetBytes = destination.GetStorage().GetRange(0, destination.GetStorage().Size());
        std::fill(targetBytes.begin(), targetBytes.end(), std::byte{0xee});
        for (std::size_t index = 0; index < sourceBytes.size(); ++index) sourceBytes[index] = static_cast<std::byte>((index * 13 + index / source.GetSlicePitch()) % 251);
        const std::vector original(sourceBytes.begin(), sourceBytes.end());
        std::vector expected(targetBytes.begin(), targetBytes.end());
        const auto region = mode == 0 ? VolumeCopyRegion{{0, 0, 0}, {0, 0, 0}, {13, 7, 5}} : VolumeCopyRegion{{1, 1, 0}, {3, 2, 3}, {5, 3, 2}};
        const auto pixelBytes = LinearTexture2D::CalculateRowBytes(1, format);
        for (std::size_t z = 0; z < region.Extent[2]; ++z)
        for (std::size_t y = 0; y < region.Extent[1]; ++y)
        {
            const auto from = (z + region.Source[2]) * source.GetSlicePitch() + (y + region.Source[1]) * source.GetRowPitch() + region.Source[0] * pixelBytes;
            const auto to = (z + region.Destination[2]) * destination.GetSlicePitch() + (y + region.Destination[1]) * destination.GetRowPitch() + region.Destination[0] * pixelBytes;
            std::copy_n(original.begin() + from, region.Extent[0] * pixelBytes, expected.begin() + to);
        }
        CommandStorage storage(16384, 2);
        auto& commands = storage.Get();
        sceAgcDcbContextStateOp(&commands, 0);
        for (std::size_t axis = 0; axis < 3; ++axis)
        {
            auto invalid = region;
            invalid.Source[axis] = std::numeric_limits<std::uint32_t>::max();
            const auto before = commands.cursor_up;
            bool rejected = false;
            try { WriteTextureCopy(commands, destination, source, invalid); }
            catch (const std::out_of_range&) { rejected = true; }
            if (!rejected || before != commands.cursor_up) throw std::runtime_error("Invalid volume copy changed commands");
        }
        const auto before = commands.cursor_up;
        bool rejected = false;
        try { WriteTextureCopy(commands, source, source, {{1, 1, 1}, {2, 2, 2}, {3, 3, 3}}); }
        catch (const std::invalid_argument&) { rejected = true; }
        if (!rejected || before != commands.cursor_up) throw std::runtime_error("Overlapping volume copy changed commands");
        WriteTextureCopy(commands, destination, source, region);
        BufferAllocation fenceStorage(8, 2);
        auto& fence = *std::construct_at(static_cast<std::uint64_t*>(fenceStorage.Data()), 0);
        SubmitAndWait(commands, fence, 1, 10000000);
        if (!std::equal(targetBytes.begin(), targetBytes.end(), expected.begin())) throw std::runtime_error("Volume copy changed incorrect pixels or padding");
        if (mode != 2 && !std::equal(sourceBytes.begin(), sourceBytes.end(), original.begin())) throw std::runtime_error("Volume copy modified its source");
    }
    std::cout << "Volume copies verified for nine formats, whole and partial regions, disjoint self copies and linear views\n";
}

void CheckVolumeUploads()
{
    using namespace AnyUnreal::RHI;
    for (const auto format : {TextureFormat::R8Unorm, TextureFormat::Rg8Unorm, TextureFormat::Rgba8Unorm, TextureFormat::Rgba8Srgb, TextureFormat::Bgra8Unorm, TextureFormat::Bgra8Srgb, TextureFormat::R32Float, TextureFormat::Rgba32Float, TextureFormat::Rgba16Float})
    for (const bool pendingWrite : {false, true})
    {
        LinearTexture3D destination(13, 7, 5, format, 2);
        const auto pixelBytes = LinearTexture2D::CalculateRowBytes(1, format);
        const auto rowBytes = 5 * pixelBytes;
        const auto rowPitch = rowBytes + 11;
        const auto slicePitch = rowPitch * 3 + 17;
        std::vector<std::byte> source(slicePitch * 2);
        for (std::size_t i = 0; i < source.size(); ++i) source[i] = static_cast<std::byte>(i % 251);
        const auto snapshot = source;
        VolumeUpload upload(5, 3, 2, format, source, rowPitch, slicePitch, 2);
        source.assign(source.size(), std::byte{0xff});
        const auto bytes = destination.GetStorage().GetRange(0, destination.GetStorage().Size());
        std::fill(bytes.begin(), bytes.end(), std::byte{0x5a});
        std::vector<std::byte> expected(bytes.size(), pendingWrite ? std::byte{0xa5} : std::byte{0x5a});
        for (std::size_t z = 0; z < 2; ++z)
        for (std::size_t y = 0; y < 3; ++y)
        {
            const auto target = (z + 2) * destination.GetSlicePitch() + (y + 1) * destination.GetRowPitch() + 3 * pixelBytes;
            std::copy_n(snapshot.begin() + z * slicePitch + y * rowPitch, rowBytes, expected.begin() + target);
        }
        CommandStorage storage(16384, 2);
        auto& commands = storage.Get();
        sceAgcDcbContextStateOp(&commands, 0);
        const auto before = commands.cursor_up;
        bool rejected = false;
        try { upload.Record(commands, destination, 12, 1, 2); }
        catch (const std::out_of_range&) { rejected = true; }
        if (!rejected || before != commands.cursor_up) throw std::runtime_error("Invalid volume upload changed commands");
        if (pendingWrite) WriteBufferFill(commands, destination.GetStorage().Data(), bytes.size(), 0xa5a5a5a5u);
        upload.Record(commands, destination, 3, 1, 2);
        const auto recorded = commands.cursor_up;
        rejected = false;
        try { upload.Record(commands, destination, 3, 1, 2); }
        catch (const std::runtime_error&) { rejected = true; }
        if (!rejected || recorded != commands.cursor_up) throw std::runtime_error("Repeated volume upload changed commands");
        BufferAllocation fenceStorage(8, 2);
        auto& fence = *std::construct_at(static_cast<std::uint64_t*>(fenceStorage.Data()), 0);
        SubmitAndWait(commands, fence, 1, 10000000);
        if (!std::equal(bytes.begin(), bytes.end(), expected.begin())) throw std::runtime_error("Volume upload corrupted slices, pixels or padding");
    }
    std::cout << "Volume uploads verified for nine formats and pending GPU writes\n";
}

int main()
{
    using namespace AnyUnreal::RHI;
    AnyUnreal::Kernel::CheckResult(sceAgcInit(11), "sceAgcInit");
    CheckVolumeCopies();
    CheckVolumeUploads();
    for (const auto format : {TextureFormat::R8Unorm, TextureFormat::Rg8Unorm, TextureFormat::Rgba8Unorm, TextureFormat::Rgba8Srgb, TextureFormat::Bgra8Unorm, TextureFormat::Bgra8Srgb, TextureFormat::R32Float, TextureFormat::Rgba32Float, TextureFormat::Rgba16Float})
    for (const unsigned mode : {0u, 1u, 2u, 3u, 4u, 5u, 6u})
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
        if (mode == 6)
        {
            for (std::size_t index = 0; index < expected.size(); ++index) expected[index] = static_cast<std::byte>((0x12345678u >> ((index % 4) * 8)) & 255);
        }
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
        if (mode == 6) WriteBufferFill(commands, destination.GetStorage().Data(), destination.GetStorage().Size(), 0x12345678u);
        std::unique_ptr<TextureUpload> upload;
        if (mode >= 5)
        {
            const auto pitch = region.Width * bytesPerPixel + 7;
            std::vector<std::byte> data(pitch * (region.Height - 1) + region.Width * bytesPerPixel);
            for (std::uint32_t row = 0; row < region.Height; ++row) std::copy_n(original.begin() + (region.SourceY + row) * source.GetRowPitch() + region.SourceX * bytesPerPixel, region.Width * bytesPerPixel, data.begin() + row * pitch);
            upload = std::make_unique<TextureUpload>(region.Width, region.Height, format, data, pitch, 2);
            std::fill(data.begin(), data.end(), std::byte{0xff});
            upload->Record(commands, destination, region.DestinationX, region.DestinationY);
        }
        else WriteTextureCopy(commands, destination, source, region);
        SubmitAndWait(commands, fence, 1, 10000000);
        if (!std::equal(expected.begin(), expected.end(), destinationBytes.begin())) throw std::runtime_error("Texture copy changed incorrect bytes or row padding");
        if (mode != 2 && !std::equal(original.begin(), original.end(), sourceBytes.begin())) throw std::runtime_error("Texture copy modified its source");
        for (const std::size_t padding : {0u, 11u})
        {
            const auto pitch = destination.GetRowBytes() + padding;
            std::vector<std::byte> readback(pitch * destination.GetHeight(), std::byte{0xee});
            destination.CopyRowsTo(readback, pitch);
            for (std::size_t row = 0; row < destination.GetHeight(); ++row)
            {
                for (std::size_t column = 0; column < destination.GetRowBytes(); ++column) if (readback[row * pitch + column] != expected[row * destination.GetRowPitch() + column]) throw std::runtime_error("Texture readback changed pixels");
                for (std::size_t column = destination.GetRowBytes(); column < pitch; ++column) if (readback[row * pitch + column] != std::byte{0xee}) throw std::runtime_error("Texture readback overwrote destination padding");
            }
        }
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
    const std::array<std::byte, 24> uploadBytes{};
    const auto rejectSource = [&](std::span<const std::byte> bytes, std::size_t pitch)
    {
        bool rejected = false;
        try { TextureUpload upload(3, 2, TextureFormat::Rgba8Unorm, bytes, pitch, 2); }
        catch (const std::invalid_argument&) { rejected = true; }
        if (!rejected) throw std::runtime_error("Invalid texture upload source was accepted");
    };
    rejectSource({}, 12);
    rejectSource(uploadBytes, 11);
    rejectSource(std::span{uploadBytes}.first(23), 12);
    TextureUpload upload(3, 2, TextureFormat::Rgba8Unorm, uploadBytes, 12, 2);
    const auto rejectUpload = [&](const LinearTexture2D& destination, std::uint32_t x, std::uint32_t y)
    {
        const auto before = commands.cursor_up;
        bool rejected = false;
        try { upload.Record(commands, destination, x, y); }
        catch (const std::invalid_argument&) { rejected = true; }
        catch (const std::out_of_range&) { rejected = true; }
        if (!rejected || commands.cursor_up != before) throw std::runtime_error("Invalid texture upload was recorded");
    };
    rejectUpload(incompatible, 0, 0);
    rejectUpload(texture, 15, 0);
    rejectUpload(texture, 0, 6);
    rejectUpload(texture, std::numeric_limits<std::uint32_t>::max(), 0);
    upload.Record(commands, texture, 1, 1);
    bool repeated = false;
    const auto before = commands.cursor_up;
    try { upload.Record(commands, texture, 1, 1); }
    catch (const std::runtime_error&) { repeated = true; }
    if (!repeated || commands.cursor_up != before) throw std::runtime_error("Texture upload was recorded twice");
    std::vector<std::byte> readback(texture.GetRowBytes() * texture.GetHeight(), std::byte{0x4f});
    const auto rejectRead = [&](std::span<std::byte> bytes, std::size_t pitch)
    {
        const std::vector original(bytes.begin(), bytes.end());
        bool rejected = false;
        try { texture.CopyRowsTo(bytes, pitch); }
        catch (const std::invalid_argument&) { rejected = true; }
        if (!rejected || !std::equal(bytes.begin(), bytes.end(), original.begin())) throw std::runtime_error("Invalid texture read was accepted or changed data");
    };
    rejectRead({}, texture.GetRowBytes());
    rejectRead(readback, texture.GetRowBytes() - 1);
    rejectRead(std::span{readback}.first(readback.size() - 1), texture.GetRowBytes());
    rejectRead(texture.GetStorage().GetRange(0, texture.GetStorage().Size()), texture.GetRowPitch());
    std::cout << "Texture copies, uploads, readbacks and invalid regions verified\n";
    return 0;
}
