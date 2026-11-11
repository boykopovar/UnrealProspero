#include "UnrealProspero/RHI/Command/DepthTarget.h"
#include "UnrealProspero/RHI/Command/BufferFill.h"
#include "prx/libSceAgc/DcbState/include/Registers.hpp"
#include "prx/libSceAgc/DcbFlow/include/Memory.hpp"
#include <array>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace UnrealProspero::RHI
{
    void WriteDepthTarget(CommandBuffer& commands, const DepthTexture2D& texture)
    {
        const auto address = reinterpret_cast<std::uintptr_t>(texture.GetStorage().Data());
        const auto low = static_cast<std::uint32_t>(address >> 8);
        const auto high = static_cast<std::uint32_t>(address >> 40);
        const std::array registers{
            ShaderRegister{0, 0}, ShaderRegister{1, 0}, ShaderRegister{2, 0},
            ShaderRegister{5, 0}, ShaderRegister{7, (texture.GetWidth() - 1) | ((texture.GetHeight() - 1) << 16)},
            ShaderRegister{0x10, 3u | (24u << 4) | (1u << 31)}, ShaderRegister{0x11, 0},
            ShaderRegister{0x12, low}, ShaderRegister{0x13, 0}, ShaderRegister{0x14, low}, ShaderRegister{0x15, 0},
            ShaderRegister{0x1a, high}, ShaderRegister{0x1b, 0}, ShaderRegister{0x1c, high}, ShaderRegister{0x1d, 0},
            ShaderRegister{0x1e, 0}, ShaderRegister{0x2af, 0}
        };
        for (const auto value : registers) sceAgcDcbSetCxRegisterDirect(&commands, value);
    }

    void WriteDepthClear(CommandBuffer& commands, const DepthTexture2D& texture, float depth)
    {
        if (!std::isfinite(depth) || depth < 0 || depth > 1) throw std::invalid_argument("Depth clear requires a finite normalized value");
        const auto& storage = texture.GetStorage();
        sceAgcDcbSetCxRegisterDirect(&commands, {0xb, std::bit_cast<std::uint32_t>(depth)});
        sceAgcDcbAcquireMem(&commands, 1, 1u << 26, 0x4381, storage.Data(), storage.Size(), 40);
        WriteBufferFill(commands, storage.Data(), storage.Size(), std::bit_cast<std::uint32_t>(depth));
        sceAgcDcbAcquireMem(&commands, 1, 1u << 26, 0x4381, storage.Data(), storage.Size(), 40);
    }

    void DisableDepthTarget(CommandBuffer& commands)
    {
        for (const auto offset : {0u, 0x10u, 0x11u, 0x200u}) sceAgcDcbSetCxRegisterDirect(&commands, {offset, 0});
    }
}
