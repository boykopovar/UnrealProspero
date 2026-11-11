#include "AnyUnreal/RHI/Pipeline/ComputePipeline.h"
#include "AnyUnreal/RHI/Command/ComputeDispatch.h"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    ComputePipeline::ComputePipeline(AnyUnreal::RHI::ComputeShader& shader) : FRHIComputePipelineState(&shader) {}

    AnyUnreal::RHI::ComputeShader& ComputePipeline::GetShader() const
    {
        return *static_cast<AnyUnreal::RHI::ComputeShader*>(GetComputeShader());
    }

    void ComputePipeline::RecordDispatch(CommandBuffer& commands, ShaderBindings& bindings, std::uint32_t groupX, std::uint32_t groupY, std::uint32_t groupZ)
    {
        if (!IsValid()) throw std::runtime_error("Compute pipeline is invalid");
        if (&bindings.GetShader() != GetComputeShader()) throw std::invalid_argument("Shader bindings do not belong to this compute pipeline");
        const auto inputs = bindings.Finalize();
        const auto& shader = GetShader();
        auto ranges = bindings.GetMemoryRanges();
        const auto& native = shader.GetNativeShader().Get();
        ranges.push_back({native.code, native.shader_size});
        WriteMemoryAcquire(commands, ranges);
        WriteComputeDispatch(commands, shader.GetNativeShader(), shader.GetCode().GetUserData(), inputs, groupX, groupY, groupZ);
        MarkUsed();
    }
}
