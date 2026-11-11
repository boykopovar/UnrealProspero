#include "AnyUnreal/RHI/Texture/SurfaceData.h"
#include "RHISurfaceDataConversion.h"
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>

namespace AnyUnreal::RHI
{
    namespace
    {
        void validate(const LinearTexture2D& texture, FIntRect rectangle, const FReadSurfaceDataFlags& flags)
        {
            if (flags.GetGPUIndex() != 0 || flags.GetMip() != 0 || flags.GetArrayIndex() != 0 || flags.GetCubeFace() != CubeFace_MAX || flags.GetOutputStencil()) throw std::invalid_argument("Surface read requires the available color subresource");
            if (flags.GetCompressionMode() < RCM_UNorm || flags.GetCompressionMode() > RCM_MinMax) throw std::invalid_argument("Invalid surface range compression mode");
            if (rectangle.Min.X < 0 || rectangle.Min.Y < 0 || rectangle.Max.X < rectangle.Min.X || rectangle.Max.Y < rectangle.Min.Y || static_cast<uint32>(rectangle.Max.X) > texture.GetWidth() || static_cast<uint32>(rectangle.Max.Y) > texture.GetHeight()) throw std::out_of_range("Surface read rectangle exceeds its texture");
        }

        uint8* source(const LinearTexture2D& texture, FIntRect rectangle)
        {
            const auto offset = static_cast<std::size_t>(rectangle.Min.Y) * texture.GetRowPitch() + rectangle.Min.X * (texture.GetRowBytes() / texture.GetWidth());
            return static_cast<uint8*>(texture.GetStorage().Data()) + offset;
        }

        std::vector<float> readFloats(const LinearTexture2D& texture, FIntRect rectangle)
        {
            const auto width = static_cast<std::size_t>(rectangle.Width());
            const auto height = static_cast<std::size_t>(rectangle.Height());
            const auto channels = texture.GetFormat() == TextureFormat::R32Float ? 1u : 4u;
            std::vector<float> values(width * height * channels);
            const auto* input = source(texture, rectangle);
            for (std::size_t row = 0; row < height; ++row) std::memcpy(values.data() + row * width * channels, input + row * texture.GetRowPitch(), width * channels * sizeof(float));
            return values;
        }

        void readBytes(const LinearTexture2D& texture, FIntRect rectangle, FColor* output)
        {
            auto* input = source(texture, rectangle);
            const auto width = static_cast<uint32>(rectangle.Width());
            const auto height = static_cast<uint32>(rectangle.Height());
            const auto pitch = static_cast<uint32>(texture.GetRowPitch());
            switch (texture.GetFormat())
            {
            case TextureFormat::R8Unorm: ConvertRawR8DataToFColor(width, height, input, pitch, output); break;
            case TextureFormat::Rg8Unorm: ConvertRawR8G8DataToFColor(width, height, input, pitch, output); break;
            case TextureFormat::Rgba8Unorm:
            case TextureFormat::Rgba8Srgb: ConvertRawR8G8B8A8DataToFColor(width, height, input, pitch, output); break;
            case TextureFormat::Bgra8Unorm:
            case TextureFormat::Bgra8Srgb: ConvertRawB8G8R8A8DataToFColor(width, height, input, pitch, output); break;
            default: throw std::invalid_argument("Surface format is not byte normalized");
            }
        }
    }

    void ReadVolumeHalfColors(const LinearTexture3D& texture, FIntRect rectangle, FIntPoint zRange, TArray<FFloat16Color>& output)
    {
        if (texture.GetFormat() != TextureFormat::Rgba16Float) throw std::invalid_argument("Half-float volume read requires an RGBA16 float texture");
        if (rectangle.Min.X < 0 || rectangle.Min.Y < 0 || rectangle.Max.X < rectangle.Min.X || rectangle.Max.Y < rectangle.Min.Y || static_cast<uint32>(rectangle.Max.X) > texture.GetWidth() || static_cast<uint32>(rectangle.Max.Y) > texture.GetHeight()) throw std::out_of_range("Volume read rectangle exceeds its texture");
        if (zRange.X < 0 || zRange.Y < zRange.X || static_cast<uint32>(zRange.Y) > texture.GetDepth()) throw std::out_of_range("Volume read slices exceed its texture");
        const auto slicePixels = static_cast<std::size_t>(rectangle.Width()) * rectangle.Height();
        const auto pixels = slicePixels * (zRange.Y - zRange.X);
        if (pixels > static_cast<std::size_t>(std::numeric_limits<int32>::max())) throw std::overflow_error("Volume read exceeds the output array range");
        TArray<FFloat16Color> result;
        result.SetNumUninitialized(static_cast<int32>(pixels));
        if (pixels != 0)
        {
            const auto offset = static_cast<std::size_t>(rectangle.Min.Y) * texture.GetRowPitch() + static_cast<std::size_t>(rectangle.Min.X) * 8;
            auto* data = static_cast<uint8*>(texture.GetStorage().Data()) + offset;
            for (int32 z = zRange.X; z < zRange.Y; ++z) ConvertRawR16G16B16A16FDataToFFloat16Color(rectangle.Width(), rectangle.Height(), data + static_cast<std::size_t>(z) * texture.GetSlicePitch(), static_cast<uint32>(texture.GetRowPitch()), result.GetData() + static_cast<std::size_t>(z - zRange.X) * slicePixels);
        }
        output = MoveTemp(result);
    }

    void ReadSurfaceHalfColors(const LinearTexture2D& texture, FIntRect rectangle, TArray<FFloat16Color>& output)
    {
        validate(texture, rectangle, FReadSurfaceDataFlags());
        if (texture.GetFormat() != TextureFormat::Rgba16Float) throw std::invalid_argument("Half-float surface read requires an RGBA16 float texture");
        TArray<FFloat16Color> result;
        result.SetNumUninitialized(rectangle.Width() * rectangle.Height());
        if (result.Num() != 0) ConvertRawR16G16B16A16FDataToFFloat16Color(rectangle.Width(), rectangle.Height(), source(texture, rectangle), static_cast<uint32>(texture.GetRowPitch()), result.GetData());
        output = MoveTemp(result);
    }

    void ReadSurfaceColors(const LinearTexture2D& texture, FIntRect rectangle, const FReadSurfaceDataFlags& flags, TArray<FColor>& output)
    {
        validate(texture, rectangle, flags);
        TArray<FColor> result;
        result.SetNumUninitialized(rectangle.Width() * rectangle.Height());
        if (result.Num() != 0)
        {
            if (texture.GetFormat() == TextureFormat::Rgba16Float)
            {
                ConvertRawR16G16B16A16FDataToFColor(rectangle.Width(), rectangle.Height(), source(texture, rectangle), static_cast<uint32>(texture.GetRowPitch()), result.GetData(), flags.GetLinearToGamma());
            }
            else if (texture.GetFormat() == TextureFormat::Rgba32Float)
            {
                auto values = readFloats(texture, rectangle);
                ConvertRawR32G32B32A32DataToFColor(rectangle.Width(), rectangle.Height(), reinterpret_cast<uint8*>(values.data()), rectangle.Width() * 4 * sizeof(float), result.GetData(), flags.GetLinearToGamma());
            }
            else if (texture.GetFormat() == TextureFormat::R32Float)
            {
                const auto values = readFloats(texture, rectangle);
                for (int32 index = 0; index < result.Num(); ++index) result[index] = FLinearColor(values[index], 0, 0, 1).ToFColor(flags.GetLinearToGamma());
            }
            else readBytes(texture, rectangle, result.GetData());
        }
        output = MoveTemp(result);
    }

    void ReadSurfaceColors(const LinearTexture2D& texture, FIntRect rectangle, const FReadSurfaceDataFlags& flags, TArray<FLinearColor>& output)
    {
        validate(texture, rectangle, flags);
        TArray<FLinearColor> result;
        result.SetNumUninitialized(rectangle.Width() * rectangle.Height());
        if (result.Num() != 0)
        {
            if (texture.GetFormat() == TextureFormat::Rgba16Float)
            {
                ConvertRawR16G16B16A16FDataToFLinearColor(rectangle.Width(), rectangle.Height(), source(texture, rectangle), static_cast<uint32>(texture.GetRowPitch()), result.GetData(), flags);
            }
            else if (texture.GetFormat() == TextureFormat::Rgba32Float)
            {
                auto values = readFloats(texture, rectangle);
                ConvertRawR32G32B32A32DataToFLinearColor(rectangle.Width(), rectangle.Height(), reinterpret_cast<uint8*>(values.data()), rectangle.Width() * 4 * sizeof(float), result.GetData(), flags);
            }
            else if (texture.GetFormat() == TextureFormat::R32Float)
            {
                const auto values = readFloats(texture, rectangle);
                for (int32 index = 0; index < result.Num(); ++index) result[index] = FLinearColor(values[index], 0, 0, 1);
            }
            else
            {
                TArray<FColor> colors;
                colors.SetNumUninitialized(result.Num());
                readBytes(texture, rectangle, colors.GetData());
                for (int32 index = 0; index < result.Num(); ++index) result[index] = FLinearColor(colors[index]);
            }
        }
        output = MoveTemp(result);
    }
}
