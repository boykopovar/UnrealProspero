#ifndef ANYUNREAL_RHI_COMPUTE_PIPELINE_H
#define ANYUNREAL_RHI_COMPUTE_PIPELINE_H

#include "AnyUnreal/RHI/Shader/ShaderBindings.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API ComputePipeline final : public FRHIComputePipelineState
    {
    public:
        explicit ComputePipeline(AnyUnreal::RHI::ComputeShader& shader);
        AnyUnreal::RHI::ComputeShader& GetShader() const;
        void RecordIndirectDispatch(CommandBuffer& commands, ShaderBindings& bindings, const BufferAllocation& arguments, std::uint32_t argumentOffset);
        void RecordDispatch(CommandBuffer& commands, ShaderBindings& bindings, std::uint32_t groupX, std::uint32_t groupY, std::uint32_t groupZ);
    private:
        Shader::UserDataInputs prepareDispatch(CommandBuffer& commands, ShaderBindings& bindings, const BufferAllocation* arguments);
    };
}

#endif
