#include "AnyUnreal/RHI/Shader/ShaderCommands.h"
#include "prx/libSceAgc/Cb/include/Registers.hpp"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    void WriteShaderRegisters(CommandBuffer& commands, const AgcShader& shader, const Shader::UserDataLayout& layout, const Shader::UserDataInputs& inputs)
    {
        const auto& native = shader.Get();
        std::uint32_t base;
        switch (layout.GetStage())
        {
            case Shader::ShaderStage::Vertex:
                if (native.type != 2) throw std::invalid_argument("Vertex shader binding has the wrong AGC stage");
                base = 0x8c;
                break;
            case Shader::ShaderStage::Pixel:
                if (native.type != 1) throw std::invalid_argument("Pixel shader binding has the wrong AGC stage");
                base = 0xc;
                break;
            case Shader::ShaderStage::Compute:
                if (native.type != 0) throw std::invalid_argument("Compute shader binding has the wrong AGC stage");
                base = 0x240;
                break;
            default: throw std::invalid_argument("Unsupported AGC shader binding stage");
        }
        const auto userData = layout.Build(inputs);
        sceAgcCbSetShRegistersDirect(&commands, native.sh_registers, native.num_sh_registers);
        if (layout.GetCount() != 0) sceAgcCbSetShRegisterRangeDirect(&commands, base, userData.data(), layout.GetCount());
    }
}
