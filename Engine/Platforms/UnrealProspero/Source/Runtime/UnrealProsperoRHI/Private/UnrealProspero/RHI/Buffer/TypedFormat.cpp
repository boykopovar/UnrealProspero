#include "UnrealProspero/RHI/Buffer/TypedFormat.h"
#include <array>
#include <stdexcept>

namespace UnrealProspero::RHI
{
    namespace
    {
        struct Mapping
        {
            EPixelFormat Pixel;
            BufferFormat Buffer;
        };

        constexpr std::array mappings{
            Mapping{PF_R32_FLOAT, BufferFormat::Float1},
            Mapping{PF_G32R32F, BufferFormat::Float2},
            Mapping{PF_A32B32G32R32F, BufferFormat::Float4},
            Mapping{PF_R32_UINT, BufferFormat::Uint1},
            Mapping{PF_G16R16F, BufferFormat::Half2},
            Mapping{PF_G16R16F_FILTER, BufferFormat::Half2},
            Mapping{PF_FloatRGBA, BufferFormat::Half4},
            Mapping{PF_R8G8B8A8, BufferFormat::Byte4Unorm},
            Mapping{PF_B8G8R8A8, BufferFormat::Bgra8Unorm},
            Mapping{PF_R8G8B8A8_UINT, BufferFormat::Byte4Uint},
            Mapping{PF_R8G8B8A8_SNORM, BufferFormat::Byte4Snorm},
            Mapping{PF_R16G16_UINT, BufferFormat::Short2Uint},
            Mapping{PF_R16G16_SINT, BufferFormat::Short2Sint},
            Mapping{PF_R16G16B16A16_UINT, BufferFormat::Short4Uint},
            Mapping{PF_R16G16B16A16_SINT, BufferFormat::Short4Sint},
            Mapping{PF_R16G16B16A16_UNORM, BufferFormat::Short4Unorm},
            Mapping{PF_R16G16B16A16_SNORM, BufferFormat::Short4Snorm}
        };
    }

    BufferFormat ResolveTypedBufferFormat(EPixelFormat format)
    {
        for (const auto& mapping : mappings)
            if (mapping.Pixel == format)
            {
                if (GPixelFormats[format].BlockBytes != static_cast<int32>(GetBufferElementSize(mapping.Buffer))) throw std::runtime_error("Typed buffer element size differs from Unreal format metadata");
                return mapping.Buffer;
            }
        throw std::runtime_error("Typed buffer pixel format is not implemented");
    }

    void ConfigureTypedBufferFormats()
    {
        using Caps = EPixelFormatCapabilities;
        for (const auto& mapping : mappings)
            GPixelFormats[mapping.Pixel].Capabilities |= Caps::Buffer | Caps::BufferLoad | Caps::BufferStore | Caps::UAV | Caps::TypedUAVLoad | Caps::TypedUAVStore;
    }
}
