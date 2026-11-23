#include "UnrealProspero/RHI/Pipeline/ComputePipeline.h"
#include "UnrealProspero/RHI/Command/ComputeDispatch.h"
#include <stdexcept>

namespace UnrealProspero::RHI
{
    ComputePipeline::ComputePipeline(UnrealProspero::RHI::ComputeShader& shader) : FRHIComputePipelineState(&shader) {}

    UnrealProspero::RHI::ComputeShader& ComputePipeline::GetShader() const
    {
        return *static_cast<UnrealProspero::RHI::ComputeShader*>(GetComputeShader());
    }

    Shader::UserDataInputs ComputePipeline::prepareDispatch(CommandBuffer& commands, ShaderBindings& bindings, const BufferAllocation* scratch, const BufferAllocation* arguments)
    {
        if (!IsValid()) throw std::runtime_error("Compute pipeline is invalid");
        if (&bindings.GetShader() != GetComputeShader()) throw std::invalid_argument("Shader bindings do not belong to this compute pipeline");
        Shader::UserDataInputs inputs;
        const auto scratchSize = GetComputeScratchAllocationSize(GetShader().GetCode().GetUserData());
        if (scratchSize != (scratch ? scratch->Size() : 0)) throw std::invalid_argument("Compute scratch allocation does not match the shader");
        if (scratch) inputs.ScratchAddress = reinterpret_cast<std::uintptr_t>(scratch->Data());
        inputs = bindings.Finalize(inputs);
        auto ranges = bindings.GetMemoryRanges();
        const auto& native = GetShader().GetNativeShader().Get();
        ranges.push_back({native.code, native.shader_size});
        if (arguments) ranges.push_back({arguments->Data(), arguments->Size()});
        WriteMemoryAcquire(commands, ranges);
        return inputs;
    }

    void ComputePipeline::RecordIndirectDispatch(CommandBuffer& commands, ShaderBindings& bindings, const BufferAllocation* scratch, const BufferAllocation& arguments, std::uint32_t argumentOffset)
    {
        arguments.GetRange(argumentOffset, 12);
        if (argumentOffset % 4 != 0) throw std::invalid_argument("Indirect dispatch argument offset must be aligned to four bytes");
        const auto inputs = prepareDispatch(commands, bindings, scratch, &arguments);
        const auto& shader = GetShader();
        WriteIndirectComputeDispatch(commands, shader.GetNativeShader(), shader.GetCode().GetUserData(), inputs, arguments.Data(), arguments.Size(), argumentOffset);
        MarkUsed();
    }

    void ComputePipeline::RecordDispatch(CommandBuffer& commands, ShaderBindings& bindings, const BufferAllocation* scratch, std::uint32_t groupX, std::uint32_t groupY, std::uint32_t groupZ)
    {
        const auto inputs = prepareDispatch(commands, bindings, scratch, nullptr);
        const auto& shader = GetShader();
        WriteComputeDispatch(commands, shader.GetNativeShader(), shader.GetCode().GetUserData(), inputs, groupX, groupY, groupZ);
        MarkUsed();
    }
}
