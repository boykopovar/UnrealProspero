#include "UnrealProspero/RHI/Command/ComputeDispatch.h"
#include "prx/libSceAgc/Cb/include/Dispatch.hpp"
#include "prx/libSceAgc/DcbDraw/include/Instancing.hpp"
#include "prx/libSceAgc/DcbFlow/include/Dispatch.hpp"
#include "prx/libSceAgc/DcbState/include/Registers.hpp"
#include <stdexcept>

namespace UnrealProspero::RHI
{
    namespace
    {
        constexpr std::uint32_t scratchWaveCapacity = 1024;
        constexpr std::uint32_t scratchWaveGranularity = 1024;
        constexpr std::uint32_t computeTemporaryRing = 0x218;

        std::uint32_t getScratchWaveUnits(const Shader::UserDataLayout& layout)
        {
            const auto units = (static_cast<std::uint64_t>(layout.GetScratchBytesPerWave()) + scratchWaveGranularity - 1) / scratchWaveGranularity;
            if (units > 0x1fff) throw std::out_of_range("AGC compute scratch exceeds the temporary ring register");
            return static_cast<std::uint32_t>(units);
        }

        void writeDispatchRegisters(CommandBuffer& commands, const AgcShader& shader, const Shader::UserDataLayout& layout, const Shader::UserDataInputs& inputs)
        {
            const auto& native = shader.Get();
            if (layout.GetStage() != Shader::ShaderStage::Compute || native.type != 0 || !native.specials) throw std::invalid_argument("Compute dispatch requires a compute shader with dispatch metadata");
            if (static_cast<std::uint32_t>(native.scratch_size_dw_per_thread) * 128 != layout.GetScratchBytesPerWave()) throw std::invalid_argument("AGC scratch requirements disagree");
            const auto units = getScratchWaveUnits(layout);
            if ((inputs.ScratchAddress != 0) != (units != 0)) throw std::invalid_argument("AGC scratch address does not match shader requirements");
            WriteShaderRegisters(commands, shader, layout, inputs);
            if (units) sceAgcDcbSetShRegisterDirect(&commands, {computeTemporaryRing, scratchWaveCapacity | (units << 12)});
        }
    }

    std::size_t GetComputeScratchAllocationSize(const Shader::UserDataLayout& layout)
    {
        return static_cast<std::size_t>(getScratchWaveUnits(layout)) * scratchWaveGranularity * scratchWaveCapacity;
    }

    void WriteIndirectComputeDispatch(CommandBuffer& commands, const AgcShader& shader, const Shader::UserDataLayout& layout, const Shader::UserDataInputs& inputs, const void* argumentBuffer, std::size_t argumentBufferSize, std::uint32_t argumentOffset)
    {
        const auto address = reinterpret_cast<std::uintptr_t>(argumentBuffer);
        if (!argumentBuffer || address % 8 != 0 || address >= (std::uint64_t{1} << 48)) throw std::invalid_argument("Invalid indirect dispatch argument buffer address");
        if (argumentOffset % 4 != 0 || argumentOffset > argumentBufferSize || argumentBufferSize - argumentOffset < 12) throw std::out_of_range("Indirect dispatch arguments exceed the buffer");
        if (argumentBufferSize > (std::uint64_t{1} << 48) - address) throw std::out_of_range("Indirect dispatch buffer exceeds GPU address space");
        const auto& native = shader.Get();
        writeDispatchRegisters(commands, shader, layout, inputs);
        sceAgcDcbSetBaseIndirectArgs(&commands, 1, argumentBuffer);
        sceAgcDcbDispatchIndirect(&commands, argumentOffset, native.specials->dispatch_modifier);
    }

    void WriteComputeDispatch(CommandBuffer& commands, const AgcShader& shader, const Shader::UserDataLayout& layout, const Shader::UserDataInputs& inputs, std::uint32_t groupX, std::uint32_t groupY, std::uint32_t groupZ)
    {
        const auto& native = shader.Get();
        writeDispatchRegisters(commands, shader, layout, inputs);
        sceAgcCbDispatch(&commands, groupX, groupY, groupZ, native.specials->dispatch_modifier);
    }
}
