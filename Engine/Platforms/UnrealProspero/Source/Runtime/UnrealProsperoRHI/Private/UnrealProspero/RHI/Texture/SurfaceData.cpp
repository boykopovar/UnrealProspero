#include "UnrealProspero/RHI/Texture/SurfaceData.h"
#include "UnrealProspero/RHI/Texture/HalfPixels.h"
#include "RHISurfaceDataConversion.h"
#include <cstring>
#include <limits>
#include <span>
#include <stdexcept>
#include <vector>

namespace UnrealProspero::RHI
{
    namespace
    {
        void validate(const Texture2D& texture, FIntRect rectangle, const FReadSurfaceDataFlags& flags)
        {
            if (texture.GetLayout() != Texture2DLayout::Linear && texture.GetLayout() != Texture2DLayout::RenderTarget64K) throw std::runtime_error("Surface readback layout is not implemented");
            const auto format = DescribeTextureFormat(texture.GetFormat());
            if (format.BlockWidth != 1 || format.BlockHeight != 1) throw std::runtime_error("Compressed surface readback is not implemented");
            if (flags.GetGPUIndex() != 0 || (flags.GetArrayIndex() < 0 || static_cast<std::uint32_t>(flags.GetArrayIndex()) >= texture.GetArrayLayers()) || flags.GetCubeFace() != CubeFace_MAX || flags.GetOutputStencil()) throw std::invalid_argument("Surface read requires the available color subresource");
            if (flags.GetCompressionMode() < RCM_UNorm || flags.GetCompressionMode() > RCM_MinMax) throw std::invalid_argument("Invalid surface range compression mode");
            const auto& mip = texture.GetMipLayout(flags.GetMip());
            if (rectangle.Min.X < 0 || rectangle.Min.Y < 0 || rectangle.Max.X < rectangle.Min.X || rectangle.Max.Y < rectangle.Min.Y || static_cast<uint32>(rectangle.Max.X) > mip.Width || static_cast<uint32>(rectangle.Max.Y) > mip.Height) throw std::out_of_range("Surface read rectangle exceeds its texture");
        }

        class SurfacePixels final
        {
        public:
            SurfacePixels(const Texture2D& texture, std::uint32_t mipLevel, std::uint32_t arrayLayer) : bytes(texture.GetMipRange(mipLevel, arrayLayer)), pitch(texture.GetMipLayout(mipLevel).RowPitch), pixelBytes(DescribeTextureFormat(texture.GetFormat()).BlockBytes)
            {
                if (texture.GetLayout() == Texture2DLayout::RenderTarget64K)
                {
                    const auto& mip = texture.GetMipLayout(mipLevel);
                    pitch = mip.RowBytes;
                    linear.resize(pitch * mip.RowCount);
                    texture.CopyRowsTo(linear, pitch, mipLevel, arrayLayer);
                    bytes = linear;
                }
            }

            std::span<std::byte> GetRange(FIntRect rectangle) const
            {
                const auto offset = static_cast<std::size_t>(rectangle.Min.Y) * pitch + static_cast<std::size_t>(rectangle.Min.X) * pixelBytes;
                if (offset > bytes.size()) throw std::out_of_range("Surface read offset exceeds its storage");
                return bytes.subspan(offset);
            }

            uint8* GetData(FIntRect rectangle) const { return reinterpret_cast<uint8*>(GetRange(rectangle).data()); }
            std::size_t GetPitch() const { return pitch; }

        private:
            std::vector<std::byte> linear;
            std::span<std::byte> bytes;
            std::size_t pitch;
            std::size_t pixelBytes;
        };

        std::vector<std::array<std::uint16_t, 4>> readHalfPixels(const Texture2D& texture, FIntRect rectangle, std::uint32_t mipLevel, std::uint32_t arrayLayer)
        {
            const auto pixelBytes = Texture2D::CalculateRowBytes(1, texture.GetFormat());
            const SurfacePixels pixels(texture, mipLevel, arrayLayer);
            const auto bytes = static_cast<std::size_t>(rectangle.Height() - 1) * pixels.GetPitch() + rectangle.Width() * pixelBytes;
            if (bytes > pixels.GetRange(rectangle).size()) throw std::out_of_range("Half surface read exceeds its storage");
            return ExpandHalfPixels(pixels.GetRange(rectangle).first(bytes), rectangle.Width(), rectangle.Height(), pixels.GetPitch(), texture.GetFormat());
        }

        std::vector<float> readFloats(const Texture2D& texture, FIntRect rectangle, std::uint32_t mipLevel, std::uint32_t arrayLayer)
        {
            const auto width = static_cast<std::size_t>(rectangle.Width());
            const auto height = static_cast<std::size_t>(rectangle.Height());
            const auto channels = texture.GetFormat() == TextureFormat::R32Float ? 1u : 4u;
            std::vector<float> values(width * height * channels);
            const SurfacePixels pixels(texture, mipLevel, arrayLayer);
            const auto* input = pixels.GetData(rectangle);
            for (std::size_t row = 0; row < height; ++row) std::memcpy(values.data() + row * width * channels, input + row * pixels.GetPitch(), width * channels * sizeof(float));
            return values;
        }

        bool isNormalized16(TextureFormat format)
        {
            return format == TextureFormat::R16Unorm || format == TextureFormat::Rg16Unorm || format == TextureFormat::Rgba16Unorm;
        }

        void readNormalized16(const Texture2D& texture, FIntRect rectangle, const FReadSurfaceDataFlags& flags, std::span<FLinearColor> output)
        {
            const SurfacePixels pixels(texture, flags.GetMip(), flags.GetArrayIndex());
            const auto* input = pixels.GetData(rectangle);
            const auto pitch = pixels.GetPitch();
            const auto channels = DescribeTextureFormat(texture.GetFormat()).BlockBytes / 2;
            for (int32 row = 0; row < rectangle.Height(); ++row)
            for (int32 column = 0; column < rectangle.Width(); ++column)
            {
                std::array<float, 4> values{0, 0, 0, 1};
                for (std::size_t channel = 0; channel < channels; ++channel)
                {
                    std::uint16_t value;
                    std::memcpy(&value, input + row * pitch + (column * channels + channel) * sizeof(value), sizeof(value));
                    values[channel] = FColor::DequantizeUNorm16ToFloat(value);
                }
                output[row * rectangle.Width() + column] = FLinearColor(values[0], values[1], values[2], values[3]);
            }
        }

        void readBytes(const Texture2D& texture, FIntRect rectangle, FColor* output, std::uint32_t mipLevel, std::uint32_t arrayLayer)
        {
            const SurfacePixels pixels(texture, mipLevel, arrayLayer);
            auto* input = pixels.GetData(rectangle);
            const auto width = static_cast<uint32>(rectangle.Width());
            const auto height = static_cast<uint32>(rectangle.Height());
            const auto pitch = static_cast<uint32>(pixels.GetPitch());
            switch (texture.GetFormat())
            {
            case TextureFormat::A8Unorm:
                for (uint32 row = 0; row < height; ++row)
                {
                    for (uint32 column = 0; column < width; ++column) output[static_cast<std::size_t>(row) * width + column] = FColor(0, 0, 0, input[static_cast<std::size_t>(row) * pitch + column]);
                }
                break;
            case TextureFormat::R8Srgb:
            case TextureFormat::R8Unorm: ConvertRawR8DataToFColor(width, height, input, pitch, output); break;
            case TextureFormat::Rg8Srgb:
            case TextureFormat::Rg8Unorm: ConvertRawR8G8DataToFColor(width, height, input, pitch, output); break;
            case TextureFormat::Rgba8Unorm:
            case TextureFormat::Rgba8Srgb: ConvertRawR8G8B8A8DataToFColor(width, height, input, pitch, output); break;
            case TextureFormat::Bgra8Unorm:
            case TextureFormat::Bgra8Srgb: ConvertRawB8G8R8A8DataToFColor(width, height, input, pitch, output); break;
            case TextureFormat::Rgb10A2Unorm: ConvertRawR10G10B10A2DataToFColor(width, height, input, pitch, output); break;
            default: throw std::invalid_argument("Surface format is not byte normalized");
            }
        }
    }

    void ReadVolumeHalfColors(const LinearTexture3D& texture, FIntRect rectangle, FIntPoint zRange, TArray<FFloat16Color>& output)
    {
        if (!IsHalfTextureFormat(texture.GetFormat())) throw std::invalid_argument("Volume read requires a half-float texture");
        if (rectangle.Min.X < 0 || rectangle.Min.Y < 0 || rectangle.Max.X < rectangle.Min.X || rectangle.Max.Y < rectangle.Min.Y || static_cast<uint32>(rectangle.Max.X) > texture.GetWidth() || static_cast<uint32>(rectangle.Max.Y) > texture.GetHeight()) throw std::out_of_range("Volume read rectangle exceeds its texture");
        if (zRange.X < 0 || zRange.Y < zRange.X || static_cast<uint32>(zRange.Y) > texture.GetDepth()) throw std::out_of_range("Volume read slices exceed its texture");
        const auto slicePixels = static_cast<std::size_t>(rectangle.Width()) * rectangle.Height();
        const auto pixels = slicePixels * (zRange.Y - zRange.X);
        if (pixels > static_cast<std::size_t>(std::numeric_limits<int32>::max())) throw std::overflow_error("Volume read exceeds the output array range");
        TArray<FFloat16Color> result;
        result.SetNumUninitialized(static_cast<int32>(pixels));
        if (pixels != 0)
        {
            const auto pixelBytes = Texture2D::CalculateRowBytes(1, texture.GetFormat());
            const auto offset = static_cast<std::size_t>(rectangle.Min.Y) * texture.GetRowPitch() + static_cast<std::size_t>(rectangle.Min.X) * pixelBytes;
            const auto bytes = static_cast<std::size_t>(rectangle.Height() - 1) * texture.GetRowPitch() + rectangle.Width() * pixelBytes;
            for (int32 z = zRange.X; z < zRange.Y; ++z)
            {
                auto values = ExpandHalfPixels(texture.GetStorage().GetRange(texture.GetMipLayout(0).Offset + offset + static_cast<std::size_t>(z) * texture.GetSlicePitch(), bytes), rectangle.Width(), rectangle.Height(), texture.GetRowPitch(), texture.GetFormat());
                ConvertRawR16G16B16A16FDataToFFloat16Color(rectangle.Width(), rectangle.Height(), reinterpret_cast<uint8*>(values.data()), rectangle.Width() * 8, result.GetData() + static_cast<std::size_t>(z - zRange.X) * slicePixels);
            }
        }
        output = MoveTemp(result);
    }

    void ReadSurfaceHalfColors(const Texture2D& texture, FIntRect rectangle, TArray<FFloat16Color>& output, std::uint32_t mipLevel, std::uint32_t arrayLayer)
    {
        if (mipLevel >= texture.GetMipCount()) throw std::out_of_range("Surface mip does not exist");
        FReadSurfaceDataFlags flags;
        flags.SetMip(static_cast<uint8>(mipLevel));
        if (arrayLayer >= texture.GetArrayLayers()) throw std::out_of_range("Surface read array layer does not exist");
        flags.SetArrayIndex(static_cast<int32>(arrayLayer));
        validate(texture, rectangle, flags);
        if (!IsHalfTextureFormat(texture.GetFormat())) throw std::invalid_argument("Surface read requires a half-float texture");
        TArray<FFloat16Color> result;
        result.SetNumUninitialized(rectangle.Width() * rectangle.Height());
        if (result.Num() != 0)
        {
            auto values = readHalfPixels(texture, rectangle, flags.GetMip(), flags.GetArrayIndex());
            ConvertRawR16G16B16A16FDataToFFloat16Color(rectangle.Width(), rectangle.Height(), reinterpret_cast<uint8*>(values.data()), rectangle.Width() * 8, result.GetData());
        }
        output = MoveTemp(result);
    }

    void ReadSurfaceColors(const Texture2D& texture, FIntRect rectangle, const FReadSurfaceDataFlags& flags, TArray<FColor>& output)
    {
        validate(texture, rectangle, flags);
        TArray<FColor> result;
        result.SetNumUninitialized(rectangle.Width() * rectangle.Height());
        if (result.Num() != 0)
        {
            if (IsHalfTextureFormat(texture.GetFormat()))
            {
                auto values = readHalfPixels(texture, rectangle, flags.GetMip(), flags.GetArrayIndex());
                ConvertRawR16G16B16A16FDataToFColor(rectangle.Width(), rectangle.Height(), reinterpret_cast<uint8*>(values.data()), rectangle.Width() * 8, result.GetData(), flags.GetLinearToGamma());
            }
            else if (isNormalized16(texture.GetFormat()))
            {
                std::vector<FLinearColor> values(result.Num());
                readNormalized16(texture, rectangle, flags, values);
                for (int32 index = 0; index < result.Num(); ++index) result[index] = values[index].ToFColor(flags.GetLinearToGamma());
            }
            else if (texture.GetFormat() == TextureFormat::R11G11B10Float)
            {
                const SurfacePixels pixels(texture, flags.GetMip(), flags.GetArrayIndex());
                auto* input = pixels.GetData(rectangle);
                const auto pitch = static_cast<uint32>(pixels.GetPitch());
                ConvertRawR11G11B10DataToFColor(rectangle.Width(), rectangle.Height(), input, pitch, result.GetData(), flags.GetLinearToGamma());
            }
            else if (texture.GetFormat() == TextureFormat::Rgba32Float)
            {
                auto values = readFloats(texture, rectangle, flags.GetMip(), flags.GetArrayIndex());
                ConvertRawR32G32B32A32DataToFColor(rectangle.Width(), rectangle.Height(), reinterpret_cast<uint8*>(values.data()), rectangle.Width() * 4 * sizeof(float), result.GetData(), flags.GetLinearToGamma());
            }
            else if (texture.GetFormat() == TextureFormat::R32Float)
            {
                const auto values = readFloats(texture, rectangle, flags.GetMip(), flags.GetArrayIndex());
                for (int32 index = 0; index < result.Num(); ++index) result[index] = FLinearColor(values[index], 0, 0, 1).ToFColor(flags.GetLinearToGamma());
            }
            else readBytes(texture, rectangle, result.GetData(), flags.GetMip(), flags.GetArrayIndex());
        }
        output = MoveTemp(result);
    }

    void ReadSurfaceColors(const Texture2D& texture, FIntRect rectangle, const FReadSurfaceDataFlags& flags, TArray<FLinearColor>& output)
    {
        validate(texture, rectangle, flags);
        TArray<FLinearColor> result;
        result.SetNumUninitialized(rectangle.Width() * rectangle.Height());
        if (result.Num() != 0)
        {
            if (IsHalfTextureFormat(texture.GetFormat()))
            {
                auto values = readHalfPixels(texture, rectangle, flags.GetMip(), flags.GetArrayIndex());
                ConvertRawR16G16B16A16FDataToFLinearColor(rectangle.Width(), rectangle.Height(), reinterpret_cast<uint8*>(values.data()), rectangle.Width() * 8, result.GetData(), flags);
            }
            else if (isNormalized16(texture.GetFormat()))
            {
                readNormalized16(texture, rectangle, flags, {result.GetData(), static_cast<std::size_t>(result.Num())});
            }
            else if (texture.GetFormat() == TextureFormat::R11G11B10Float)
            {
                const SurfacePixels pixels(texture, flags.GetMip(), flags.GetArrayIndex());
                auto* input = pixels.GetData(rectangle);
                const auto pitch = static_cast<uint32>(pixels.GetPitch());
                ConvertRawR11G11B10FDataToFLinearColor(rectangle.Width(), rectangle.Height(), input, pitch, result.GetData());
            }
            else if (texture.GetFormat() == TextureFormat::Rgba32Float)
            {
                auto values = readFloats(texture, rectangle, flags.GetMip(), flags.GetArrayIndex());
                ConvertRawR32G32B32A32DataToFLinearColor(rectangle.Width(), rectangle.Height(), reinterpret_cast<uint8*>(values.data()), rectangle.Width() * 4 * sizeof(float), result.GetData(), flags);
            }
            else if (texture.GetFormat() == TextureFormat::R32Float)
            {
                const auto values = readFloats(texture, rectangle, flags.GetMip(), flags.GetArrayIndex());
                for (int32 index = 0; index < result.Num(); ++index) result[index] = FLinearColor(values[index], 0, 0, 1);
            }
            else if (texture.GetFormat() == TextureFormat::Rgb10A2Unorm)
            {
                const SurfacePixels pixels(texture, flags.GetMip(), flags.GetArrayIndex());
                const auto* input = pixels.GetData(rectangle);
                for (int32 row = 0; row < rectangle.Height(); ++row)
                {
                    for (int32 column = 0; column < rectangle.Width(); ++column)
                    {
                        std::uint32_t value;
                        std::memcpy(&value, input + static_cast<std::size_t>(row) * pixels.GetPitch() + static_cast<std::size_t>(column) * 4, sizeof(value));
                        result[row * rectangle.Width() + column] = FLinearColor((value & 1023u) / 1023.0f, ((value >> 10) & 1023u) / 1023.0f, ((value >> 20) & 1023u) / 1023.0f, (value >> 30) / 3.0f);
                    }
                }
            }
            else
            {
                TArray<FColor> colors;
                colors.SetNumUninitialized(result.Num());
                readBytes(texture, rectangle, colors.GetData(), flags.GetMip(), flags.GetArrayIndex());
                for (int32 index = 0; index < result.Num(); ++index) result[index] = FLinearColor(colors[index]);
            }
        }
        output = MoveTemp(result);
    }
}
