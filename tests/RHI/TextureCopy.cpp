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

void CheckVolumeMipData()
{
    using namespace AnyUnreal::RHI;
    for (const auto format : {TextureFormat::R8Unorm, TextureFormat::Rg8Unorm, TextureFormat::Rgba8Unorm, TextureFormat::Rgba16Float, TextureFormat::Rgba32Float})
    for (const auto dimensions : {std::array{13u, 7u, 5u}, std::array{1u, 1u, 8u}})
    {
        LinearTexture3D texture(dimensions[0], dimensions[1], dimensions[2], format, 2, 4);
        auto allocation = texture.GetStorage().GetRange(0, texture.GetStorage().Size());
        std::fill(allocation.begin(), allocation.end(), std::byte{0xa5});
        std::vector expected(allocation.begin(), allocation.end());
        for (std::uint32_t level = 0; level < 4; ++level)
        {
            const auto& mip = texture.GetMipLayout(level);
            const auto rowPitch = mip.RowBytes + 3;
            const auto slicePitch = rowPitch * mip.Height + 5;
            std::vector<std::byte> input(slicePitch * mip.Depth);
            for (std::size_t index = 0; index < input.size(); ++index) input[index] = static_cast<std::byte>((index * 17 + level * 29) & 255);
            texture.Upload(input, rowPitch, slicePitch, level);
            for (std::size_t z = 0; z < mip.Depth; ++z)
            for (std::size_t y = 0; y < mip.Height; ++y) std::copy_n(input.begin() + z * slicePitch + y * rowPitch, mip.RowBytes, expected.begin() + mip.Offset + z * texture.GetSlicePitch() + y * mip.RowPitch);
            if (!std::equal(allocation.begin(), allocation.end(), expected.begin())) throw std::runtime_error("Volume mip upload changed another level or padding");
            std::vector<std::byte> output(input.size(), std::byte{0xcc});
            texture.CopyTo(output, rowPitch, slicePitch, level);
            for (std::size_t z = 0; z < mip.Depth; ++z)
            for (std::size_t index = 0; index < slicePitch; ++index)
            {
                const bool pixel = index / rowPitch < mip.Height && index % rowPitch < mip.RowBytes;
                if (output[z * slicePitch + index] != (pixel ? input[z * slicePitch + index] : std::byte{0xcc})) throw std::runtime_error("Volume mip readback changed pixels or padding");
            }
            bool rejected = false;
            try { texture.GetSliceRange(level, mip.Depth); }
            catch (const std::out_of_range&) { rejected = true; }
            if (!rejected) throw std::runtime_error("Volume mip accepted an unavailable slice");
        }
    }
}

void CheckVolumeCopies()
{
    using namespace AnyUnreal::RHI;
    for (const auto format : {TextureFormat::R8Unorm, TextureFormat::Rg8Unorm, TextureFormat::Rgba8Unorm, TextureFormat::Rgba8Srgb, TextureFormat::Bgra8Unorm, TextureFormat::Bgra8Srgb, TextureFormat::R32Float, TextureFormat::Rgba32Float, TextureFormat::Rgba16Float, TextureFormat::R16Float, TextureFormat::Rg16Float})
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
        const std::vector beforeUpload(sourceBytes.begin(), sourceBytes.end());
        for (const auto offset : {std::size_t{0}, std::size_t{1}})
        {
            bool rejected = false;
            try { source.Upload(sourceBytes.subspan(offset), source.GetRowPitch(), source.GetSlicePitch()); }
            catch (const std::invalid_argument&) { rejected = true; }
            if (!rejected || !std::equal(sourceBytes.begin(), sourceBytes.end(), beforeUpload.begin())) throw std::runtime_error("Overlapping volume upload was accepted or modified its allocation");
        }
    }
    std::cout << "Volume copies verified for eleven formats, whole and partial regions, disjoint self copies and linear views\n";
}

void CheckVolumeUploads()
{
    using namespace AnyUnreal::RHI;
    for (const auto format : {TextureFormat::R8Unorm, TextureFormat::Rg8Unorm, TextureFormat::Rgba8Unorm, TextureFormat::Rgba8Srgb, TextureFormat::Bgra8Unorm, TextureFormat::Bgra8Srgb, TextureFormat::R32Float, TextureFormat::Rgba32Float, TextureFormat::Rgba16Float, TextureFormat::R16Float, TextureFormat::Rg16Float})
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
    std::cout << "Volume uploads verified for eleven formats and pending GPU writes\n";
}

void CheckMipCopies()
{
    using namespace AnyUnreal::RHI;
    for (const auto format : {TextureFormat::R8Unorm, TextureFormat::Rg8Unorm, TextureFormat::Rgba8Unorm, TextureFormat::Bgra8Unorm, TextureFormat::R32Float, TextureFormat::Rgba32Float, TextureFormat::Rgba16Float, TextureFormat::R16Float, TextureFormat::Rg16Float})
    for (const auto mode : {0u, 1u, 2u, 3u})
    {
        LinearTexture2D source(67, 19, format, 2, AnyUnreal::Kernel::DirectAllocation::PageSize, 6);
        LinearTexture2D other(91, 37, format, 2, AnyUnreal::Kernel::DirectAllocation::PageSize, 7);
        const auto& destination = mode == 1 ? source : other;
        const auto sourceBytes = source.GetStorage().GetRange(0, source.GetStorage().Size());
        const auto destinationBytes = destination.GetStorage().GetRange(0, destination.GetStorage().Size());
        std::fill(destinationBytes.begin(), destinationBytes.end(), std::byte{0xa7});
        for (std::size_t index = 0; index < sourceBytes.size(); ++index) sourceBytes[index] = static_cast<std::byte>((index * 13 + index / 256) % 251);
        const std::vector original(sourceBytes.begin(), sourceBytes.end());
        std::vector expected(destinationBytes.begin(), destinationBytes.end());
        std::vector<TextureCopyRegion> regions;
        if (mode == 0 || mode == 3) regions.push_back({1, 1, 2, 0, 6, 3, 2, 3});
        else if (mode == 1) regions.push_back({0, 0, 1, 1, 4, 2, 3, 2});
        else
        {
            for (std::uint32_t mip = 2; mip < 6; ++mip)
            {
                const auto& level = source.GetMipLayout(mip);
                regions.push_back({0, 0, 0, 0, level.Width, level.Height, mip, mip - 1});
            }
        }
        const auto pixelBytes = LinearTexture2D::CalculateRowBytes(1, format);
        for (const auto& region : regions)
        {
            const auto& from = source.GetMipLayout(region.SourceMip);
            const auto& to = destination.GetMipLayout(region.DestinationMip);
            for (std::size_t row = 0; row < region.Height; ++row)
            {
                std::copy_n(original.begin() + from.Offset + (region.SourceY + row) * from.RowPitch + region.SourceX * pixelBytes, region.Width * pixelBytes, expected.begin() + to.Offset + (region.DestinationY + row) * to.RowPitch + region.DestinationX * pixelBytes);
            }
        }
        BufferAllocation fenceStorage(8, 2);
        auto& fence = *std::construct_at(static_cast<std::uint64_t*>(fenceStorage.Data()), 0);
        CommandStorage storage(16384, 2);
        auto& commands = storage.Get();
        sceAgcDcbContextStateOp(&commands, 0);
        std::unique_ptr<TextureUpload> upload;
        if (mode == 3)
        {
            const auto& region = regions.front();
            const auto& from = source.GetMipLayout(region.SourceMip);
            const auto pitch = region.Width * pixelBytes + 7;
            std::vector<std::byte> pixels(pitch * region.Height, std::byte{0xbb});
            for (std::size_t row = 0; row < region.Height; ++row) std::copy_n(original.begin() + from.Offset + (region.SourceY + row) * from.RowPitch + region.SourceX * pixelBytes, region.Width * pixelBytes, pixels.begin() + row * pitch);
            upload = std::make_unique<TextureUpload>(region.Width, region.Height, format, pixels, pitch, 2);
            std::fill(pixels.begin(), pixels.end(), std::byte{0xff});
            upload->Record(commands, destination, region.DestinationX, region.DestinationY, region.DestinationMip);
        }
        else for (const auto& region : regions) WriteTextureCopy(commands, destination, source, region);
        SubmitAndWait(commands, fence, 1, 10000000);
        if (!std::equal(expected.begin(), expected.end(), destinationBytes.begin())) throw std::runtime_error("Mip copy changed adjacent levels, padding or incorrect texels");
        if (mode != 1 && !std::equal(original.begin(), original.end(), sourceBytes.begin())) throw std::runtime_error("Mip copy changed its source");
    }
}

int main()
{
    {
        using namespace AnyUnreal::RHI;
        const auto volumeLevels = CalculateLinearMipLayout(13, 7, 5, 4, 4);
        const std::size_t volumeOffsets[]{1792, 768, 256, 0};
        const std::uint32_t volumeDepths[]{5, 2, 1, 1};
        for (std::size_t level = 0; level < volumeLevels.size(); ++level) if (volumeLevels[level].Offset != volumeOffsets[level] || volumeLevels[level].Depth != volumeDepths[level]) throw std::runtime_error("Volume mip slice layout is incorrect");
        const auto deepLevels = CalculateLinearMipLayout(1, 1, 8, 4, 4);
        if (deepLevels.front().Offset + deepLevels.front().Size != 1024 || deepLevels.back().Depth != 1) throw std::runtime_error("Depth-dominant mip layout is incorrect");
        const auto levels = LinearTexture2D::CalculateMipLayout(7, 5, TextureFormat::Rgba8Unorm, 3);
        const std::size_t tailSizes[]{2560, 1280, 512};
        for (std::uint32_t firstMip = 0; firstMip < 3; ++firstMip) if (LinearTexture2D::CalculateDataSize(7, 5, TextureFormat::Rgba8Unorm, 3, firstMip) != tailSizes[firstMip]) throw std::runtime_error("Texture mip tail size is incorrect");
        bool invalidTailRejected = false;
        try { LinearTexture2D::CalculateDataSize(7, 5, TextureFormat::Rgba8Unorm, 3, 3); }
        catch (const std::out_of_range&) { invalidTailRejected = true; }
        if (!invalidTailRejected) throw std::runtime_error("Texture size accepted an unavailable mip");
        const std::array<std::size_t, 3> offsets{1280, 512, 0};
        const std::array<std::size_t, 3> sizes{1280, 768, 512};
        for (std::size_t level = 0; level < levels.size(); ++level)
        {
            const auto& mip = levels[level];
            if (mip.Offset != offsets[level] || mip.Size != sizes[level] || mip.RowPitch != 256 || mip.Width != (7u >> level) || mip.Height != (5u >> level) || mip.RowBytes != mip.Width * 4) throw std::runtime_error("Odd-sized mip layout does not match aligned reverse allocation");
        }
        for (std::uint32_t width : {1u, 2u, 255u, 256u, 16384u})
        for (const auto format : {TextureFormat::R8Unorm, TextureFormat::Rg8Unorm, TextureFormat::Rgba8Unorm, TextureFormat::Rgba16Float, TextureFormat::Rgba32Float})
        {
            const auto single = LinearTexture2D::CalculateMipLayout(width, 3, format, 1).front();
            if (single.Offset != 0 || single.Size != LinearTexture2D::CalculateDataSize(width, 3, format) || single.RowPitch != LinearTexture2D::CalculateRowPitch(width, format)) throw std::runtime_error("Single mip layout changed existing texture geometry");
        }
        for (std::uint32_t count : {0u, 4u, 32u, 0xffffffffu})
        {
            bool rejected = false;
            try { LinearTexture2D::CalculateMipLayout(7, 5, TextureFormat::Rgba8Unorm, count); }
            catch (const std::invalid_argument&) { rejected = true; }
            if (!rejected) throw std::runtime_error("Invalid mip count was accepted");
        }
    }

    using namespace AnyUnreal::RHI;
    AnyUnreal::Kernel::CheckResult(sceAgcInit(11), "sceAgcInit");
    const auto initialTextureBytes = BufferAllocation::GetTextureAllocationBytes();
    const auto initialNonStreamingBytes = BufferAllocation::GetNonStreamingTextureAllocationBytes();
    {
        LinearTexture2D streaming(67, 19, TextureFormat::Rgba8Unorm, 2, AnyUnreal::Kernel::DirectAllocation::PageSize, 6, AllocationUsage::StreamingTexture);
        if (BufferAllocation::GetTextureAllocationBytes() != initialTextureBytes + streaming.GetStorage().AllocationSize() || BufferAllocation::GetNonStreamingTextureAllocationBytes() != initialNonStreamingBytes) throw std::runtime_error("Streaming texture allocation entered the non-streaming budget");
        LinearTexture2D regular(7, 5, TextureFormat::Rgba8Unorm, 2);
        if (BufferAllocation::GetTextureAllocationBytes() != initialTextureBytes + streaming.GetStorage().AllocationSize() + regular.GetStorage().AllocationSize() || BufferAllocation::GetNonStreamingTextureAllocationBytes() != initialNonStreamingBytes + regular.GetStorage().AllocationSize()) throw std::runtime_error("Non-streaming texture allocation budget is incorrect");
    }
    if (BufferAllocation::GetTextureAllocationBytes() != initialTextureBytes || BufferAllocation::GetNonStreamingTextureAllocationBytes() != initialNonStreamingBytes) throw std::runtime_error("Released textures remain in the allocation budget");
    CheckMipCopies();
    CheckVolumeMipData();
    CheckVolumeCopies();
    CheckVolumeUploads();
    for (const auto format : {TextureFormat::R8Unorm, TextureFormat::Rg8Unorm, TextureFormat::Rgba8Unorm, TextureFormat::Rgba8Srgb, TextureFormat::Bgra8Unorm, TextureFormat::Bgra8Srgb, TextureFormat::R32Float, TextureFormat::Rgba32Float, TextureFormat::Rgba16Float, TextureFormat::R16Float, TextureFormat::Rg16Float})
    for (const auto mipCount : {1u, 3u})
    for (const unsigned mode : {0u, 1u, 2u, 3u, 4u, 5u, 6u})
    {
        LinearTexture2D source(67, 9, format, 2, AnyUnreal::Kernel::DirectAllocation::PageSize, mipCount);
        auto destinationFormat = format;
        if (mode == 4 && format == TextureFormat::Rgba8Srgb) destinationFormat = TextureFormat::Rgba8Unorm;
        if (mode == 4 && format == TextureFormat::Bgra8Srgb) destinationFormat = TextureFormat::Bgra8Unorm;
        LinearTexture2D other(91, 13, destinationFormat, 2, AnyUnreal::Kernel::DirectAllocation::PageSize, mipCount == 1 ? 1u : 4u);
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
            const auto from = source.GetMipLayout(0).Offset + (region.SourceY + row) * source.GetRowPitch() + region.SourceX * bytesPerPixel;
            const auto to = destination.GetMipLayout(0).Offset + (region.DestinationY + row) * destination.GetRowPitch() + region.DestinationX * bytesPerPixel;
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
            for (std::uint32_t row = 0; row < region.Height; ++row) std::copy_n(original.begin() + source.GetMipLayout(0).Offset + (region.SourceY + row) * source.GetRowPitch() + region.SourceX * bytesPerPixel, region.Width * bytesPerPixel, data.begin() + row * pitch);
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
                for (std::size_t column = 0; column < destination.GetRowBytes(); ++column) if (readback[row * pitch + column] != expected[destination.GetMipLayout(0).Offset + row * destination.GetRowPitch() + column]) throw std::runtime_error("Texture readback changed pixels");
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
    reject(texture, {0, 0, 0, 0, 1, 1, 1, 0});
    reject(texture, {0, 0, 0, 0, 1, 1, 0, 1});
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
    const auto rejectUpload = [&](const LinearTexture2D& destination, std::uint32_t x, std::uint32_t y, std::uint32_t mipLevel = 0)
    {
        const auto before = commands.cursor_up;
        bool rejected = false;
        try { upload.Record(commands, destination, x, y, mipLevel); }
        catch (const std::invalid_argument&) { rejected = true; }
        catch (const std::out_of_range&) { rejected = true; }
        if (!rejected || commands.cursor_up != before) throw std::runtime_error("Invalid texture upload was recorded");
    };
    rejectUpload(incompatible, 0, 0);
    rejectUpload(texture, 0, 0, 1);
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
