#ifndef ANYUNREAL_RHI_COMPUTE_CONTEXT_H
#define ANYUNREAL_RHI_COMPUTE_CONTEXT_H

#include "AnyUnreal/RHI/Pipeline/ComputeParameters.h"
#include "AnyUnreal/RHI/Pipeline/ComputePipeline.h"
#include "RHIContext.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API ComputeContext : public IRHIComputeContext
    {
    public:
        void RHISetComputePipelineState(FRHIComputePipelineState* pipelineState) override;
        void RHISetShaderParameters(FRHIComputeShader* computeShader, TConstArrayView<uint8> parametersData, TConstArrayView<FRHIShaderParameter> shaderParameters, TConstArrayView<FRHIShaderParameterResource> resourceParameters, TConstArrayView<FRHIShaderParameterResource> bindlessParameters) override;
        void RHIDispatchComputeShader(uint32 groupX, uint32 groupY, uint32 groupZ) override;

    protected:
        explicit ComputeContext(CommandBuffer& commands);

    private:
        struct Dispatch
        {
            TRefCountPtr<ComputePipeline> pipeline;
            std::unique_ptr<ShaderBindings> bindings;
        };

        void requireWritable() const;
        CommandBuffer& commands;
        TRefCountPtr<ComputePipeline> pipeline;
        std::unique_ptr<ComputeParameters> parameters;
        std::vector<Dispatch> dispatches;
        bool failed = false;
    };
}

#endif
