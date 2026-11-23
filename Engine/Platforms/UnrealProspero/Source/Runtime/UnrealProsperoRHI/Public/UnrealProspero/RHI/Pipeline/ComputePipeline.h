#ifndef UNREALPROSPERO_RHI_COMPUTE_PIPELINE_H
#define UNREALPROSPERO_RHI_COMPUTE_PIPELINE_H

#include "UnrealProspero/RHI/Shader/ShaderBindings.h"

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API ComputePipeline final : public FRHIComputePipelineState
    {
    public:
        explicit ComputePipeline(UnrealProspero::RHI::ComputeShader& shader);
        UnrealProspero::RHI::ComputeShader& GetShader() const;
        void RecordIndirectDispatch(CommandBuffer& commands, ShaderBindings& bindings, const BufferAllocation* scratch, const BufferAllocation& arguments, std::uint32_t argumentOffset);
        void RecordDispatch(CommandBuffer& commands, ShaderBindings& bindings, const BufferAllocation* scratch, std::uint32_t groupX, std::uint32_t groupY, std::uint32_t groupZ);
    private:
        Shader::UserDataInputs prepareDispatch(CommandBuffer& commands, ShaderBindings& bindings, const BufferAllocation* scratch, const BufferAllocation* arguments);
    };
}

#endif
