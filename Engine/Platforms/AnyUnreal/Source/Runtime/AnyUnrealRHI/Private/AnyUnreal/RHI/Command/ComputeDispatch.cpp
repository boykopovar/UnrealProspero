#include "AnyUnreal/RHI/Command/ComputeDispatch.h"
#include "prx/libSceAgc/Cb/include/Dispatch.hpp"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    void WriteComputeDispatch(CommandBuffer& commands, const AgcShader& shader, const Shader::UserDataLayout& layout, const Shader::UserDataInputs& inputs, std::uint32_t groupX, std::uint32_t groupY, std::uint32_t groupZ)
    {
        const auto& native = shader.Get();
        if (layout.GetStage() != Shader::ShaderStage::Compute || native.type != 0 || !native.specials) throw std::invalid_argument("Compute dispatch requires a compute shader with dispatch metadata");
        WriteShaderRegisters(commands, shader, layout, inputs);
        sceAgcCbDispatch(&commands, groupX, groupY, groupZ, native.specials->dispatch_modifier);
    }
}
