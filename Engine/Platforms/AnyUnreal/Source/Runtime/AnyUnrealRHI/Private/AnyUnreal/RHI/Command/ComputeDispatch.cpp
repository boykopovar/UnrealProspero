#include "AnyUnreal/RHI/Command/ComputeDispatch.h"
#include "prx/libSceAgc/Cb/include/Dispatch.hpp"
#include "prx/libSceAgc/DcbDraw/include/Instancing.hpp"
#include "prx/libSceAgc/DcbFlow/include/Dispatch.hpp"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    void WriteIndirectComputeDispatch(CommandBuffer& commands, const AgcShader& shader, const Shader::UserDataLayout& layout, const Shader::UserDataInputs& inputs, const void* argumentBuffer, std::size_t argumentBufferSize, std::uint32_t argumentOffset)
    {
        const auto address = reinterpret_cast<std::uintptr_t>(argumentBuffer);
        if (!argumentBuffer || address % 8 != 0 || address >= (std::uint64_t{1} << 48)) throw std::invalid_argument("Invalid indirect dispatch argument buffer address");
        if (argumentOffset % 4 != 0 || argumentOffset > argumentBufferSize || argumentBufferSize - argumentOffset < 12) throw std::out_of_range("Indirect dispatch arguments exceed the buffer");
        if (argumentBufferSize > (std::uint64_t{1} << 48) - address) throw std::out_of_range("Indirect dispatch buffer exceeds GPU address space");
        const auto& native = shader.Get();
        if (layout.GetStage() != Shader::ShaderStage::Compute || native.type != 0 || !native.specials) throw std::invalid_argument("Indirect compute dispatch requires a compute shader with dispatch metadata");
        WriteShaderRegisters(commands, shader, layout, inputs);
        sceAgcDcbSetBaseIndirectArgs(&commands, 1, argumentBuffer);
        sceAgcDcbDispatchIndirect(&commands, argumentOffset, native.specials->dispatch_modifier);
    }

    void WriteComputeDispatch(CommandBuffer& commands, const AgcShader& shader, const Shader::UserDataLayout& layout, const Shader::UserDataInputs& inputs, std::uint32_t groupX, std::uint32_t groupY, std::uint32_t groupZ)
    {
        const auto& native = shader.Get();
        if (layout.GetStage() != Shader::ShaderStage::Compute || native.type != 0 || !native.specials) throw std::invalid_argument("Compute dispatch requires a compute shader with dispatch metadata");
        WriteShaderRegisters(commands, shader, layout, inputs);
        sceAgcCbDispatch(&commands, groupX, groupY, groupZ, native.specials->dispatch_modifier);
    }
}
