#ifndef ANYUNREAL_RHI_COMPUTE_CONTEXT_H
#define ANYUNREAL_RHI_COMPUTE_CONTEXT_H

#include "AnyUnreal/RHI/Shader/ShaderParameters.h"
#include "AnyUnreal/RHI/Pipeline/ComputePipeline.h"
#include "RHIContext.h"
#include "AnyUnreal/RHI/Buffer/StaticUniformBuffers.h"
#include "AnyUnreal/RHI/Transition/ResourceTransition.h"
#include "AnyUnreal/RHI/Command/CommandStorage.h"
#include "AnyUnreal/RHI/Command/Completion.h"
#include "AnyUnreal/RHI/Memory/Readback.h"
#include "AnyUnreal/RHI/Memory/Upload.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API ComputeContext : public IRHICommandContext
    {
    public:
        ~ComputeContext() noexcept override = default;
        using IRHICommandContext::RHISetShaderParameters;
        ERHIPipeline GetPipeline() const override { return ERHIPipeline::Graphics; }
#if WITH_RHI_BREADCRUMBS
        void RHIBeginBreadcrumbGPU(FRHIBreadcrumbNode* breadcrumb) override;
        void RHIEndBreadcrumbGPU(FRHIBreadcrumbNode* breadcrumb) override;
#endif
        void RHIBeginTransitions(TArrayView<const FRHITransition*> transitions) override;
        void RHIEndTransitions(TArrayView<const FRHITransition*> transitions) override;
        void RHISetComputePipelineState(FRHIComputePipelineState* pipelineState) override;
        void RHISetShaderParameters(FRHIComputeShader* computeShader, TConstArrayView<uint8> parametersData, TConstArrayView<FRHIShaderParameter> shaderParameters, TConstArrayView<FRHIShaderParameterResource> resourceParameters, TConstArrayView<FRHIShaderParameterResource> bindlessParameters) override;
        void RHIClearUAVUint(FRHIUnorderedAccessView* view, const FUintVector4& values) override;
        void RHIClearUAVFloat(FRHIUnorderedAccessView* view, const FVector4f& values) override;
        void RHICopyBufferRegion(FRHIBuffer* destination, uint64 destinationOffset, FRHIBuffer* source, uint64 sourceOffset, uint64 size) override;
        void RHIWriteGPUFence(FRHIGPUFence* fence) override;
        void RHICopyToStagingBuffer(FRHIBuffer* source, FRHIStagingBuffer* destination, uint32 offset, uint32 size) override;
        void UploadBuffer(FRHIBuffer* destination, std::size_t offset, std::span<const std::byte> source);
        void RHIDispatchIndirectComputeShader(FRHIBuffer* argumentBuffer, uint32 argumentOffset) override;
        void RHIDispatchComputeShader(uint32 groupX, uint32 groupY, uint32 groupZ) override;
        void RHISetStaticUniformBuffers(const FUniformBufferStaticBindings& uniformBuffers) override;
        void RHISetStaticUniformBuffer(FUniformBufferStaticSlot slot, FRHIUniformBuffer* uniformBuffer) override;

    protected:
        ComputeContext(std::size_t initialCommandBytes, std::uint32_t addressHigh);
        void ApplyStaticUniformBuffers(ShaderParameters& shaderParameters) const;
        virtual void ValidateFinalization() const {}
        std::uint32_t GetAddressHigh() const { return addressHigh; }

        template<typename TFunction>
        void RecordCommands(TFunction&& function)
        {
            requireWritable();
            try
            {
                function(commands);
            }
            catch (...)
            {
                failed = true;
                throw;
            }
        }

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
        Completion completion;
        CommandBuffer& commands;
        TRefCountPtr<ComputePipeline> pipeline;
        std::unique_ptr<ShaderParameters> parameters;
        std::vector<Dispatch> dispatches;
        StaticUniformBuffers staticUniforms;
        std::vector<std::shared_ptr<BufferAllocation>> clearedBuffers;
        std::vector<std::shared_ptr<ResourceTransition>> retainedTransitions;
        std::vector<std::shared_ptr<BufferAllocation>> copiedBuffers;
        std::vector<std::shared_ptr<Completion>> fenceMarkers;
        std::vector<std::shared_ptr<Readback>> readbacks;
        std::vector<std::shared_ptr<Upload>> uploads;
        std::uint32_t addressHigh;
        bool failed = false;
        bool finalized = false;
    };
}

#endif
