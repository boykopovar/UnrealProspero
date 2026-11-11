#ifndef ANYUNREAL_RHI_COMPUTE_CONTEXT_H
#define ANYUNREAL_RHI_COMPUTE_CONTEXT_H

#include "AnyUnreal/RHI/Pipeline/ComputeParameters.h"
#include "AnyUnreal/RHI/Pipeline/ComputePipeline.h"
#include "RHIContext.h"
#include "AnyUnreal/RHI/Buffer/StaticUniformBuffers.h"
#include "AnyUnreal/RHI/Transition/BufferTransition.h"
#include "AnyUnreal/RHI/Command/CommandStorage.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API ComputeContext : public IRHICommandContext
    {
    public:
        using IRHICommandContext::RHISetShaderParameters;
        ERHIPipeline GetPipeline() const override { return ERHIPipeline::Graphics; }
        void RHIBeginTransitions(TArrayView<const FRHITransition*> transitions) override;
        void RHIEndTransitions(TArrayView<const FRHITransition*> transitions) override;
        void RHISetComputePipelineState(FRHIComputePipelineState* pipelineState) override;
        void RHISetShaderParameters(FRHIComputeShader* computeShader, TConstArrayView<uint8> parametersData, TConstArrayView<FRHIShaderParameter> shaderParameters, TConstArrayView<FRHIShaderParameterResource> resourceParameters, TConstArrayView<FRHIShaderParameterResource> bindlessParameters) override;
        void RHIClearUAVUint(FRHIUnorderedAccessView* view, const FUintVector4& values) override;
        void RHIClearUAVFloat(FRHIUnorderedAccessView* view, const FVector4f& values) override;
        void RHIDispatchIndirectComputeShader(FRHIBuffer* argumentBuffer, uint32 argumentOffset) override;
        void RHIDispatchComputeShader(uint32 groupX, uint32 groupY, uint32 groupZ) override;
        void RHISetStaticUniformBuffers(const FUniformBufferStaticBindings& uniformBuffers) override;
        void RHISetStaticUniformBuffer(FUniformBufferStaticSlot slot, FRHIUniformBuffer* uniformBuffer) override;

    protected:
        ComputeContext(std::size_t initialCommandBytes, std::uint32_t addressHigh);

    private:
        friend class CommandList;
        Packet finalize();
        struct Dispatch
        {
            TRefCountPtr<ComputePipeline> pipeline;
            std::unique_ptr<ShaderBindings> bindings;
            std::shared_ptr<BufferAllocation> arguments;
        };

        void requireWritable() const;
        void recordTransitions(TArrayView<const FRHITransition*> transitions, bool begin);
        void clearBufferView(FRHIUnorderedAccessView* view, std::uint32_t value);
        std::unique_ptr<ShaderBindings> createDispatchBindings() const;
        CommandStorage commandStorage;
        CommandBuffer& commands;
        TRefCountPtr<ComputePipeline> pipeline;
        std::unique_ptr<ComputeParameters> parameters;
        std::vector<Dispatch> dispatches;
        StaticUniformBuffers staticUniforms;
        std::vector<TRefCountPtr<FRHIUnorderedAccessView>> clearedViews;
        std::vector<std::shared_ptr<BufferTransition>> retainedTransitions;
        bool failed = false;
        bool finalized = false;
    };
}

#endif
