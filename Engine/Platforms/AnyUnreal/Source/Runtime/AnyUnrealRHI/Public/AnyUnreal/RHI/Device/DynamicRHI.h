#ifndef ANYUNREAL_RHI_DYNAMIC_RHI_H
#define ANYUNREAL_RHI_DYNAMIC_RHI_H

#include "AnyUnreal/Shader/ShaderBinary.h"
#include "AnyUnreal/RHI/Command/CommandList.h"
#include <DynamicRHI.h>
#include <vector>

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API DynamicRHI : public FDynamicRHI
    {
    public:
        FVertexShaderRHIRef RHICreateVertexShader(const FRHICreateShaderDesc& description) override;
        FPixelShaderRHIRef RHICreatePixelShader(const FRHICreateShaderDesc& description) override;
        FComputeShaderRHIRef RHICreateComputeShader(const FRHICreateShaderDesc& description) override;
        FComputePipelineStateRHIRef RHICreateComputePipelineState(const FComputePipelineStateInitializer& initializer) override;
        FUniformBufferRHIRef RHICreateUniformBuffer(const void* contents, const FRHIUniformBufferLayout* layout, EUniformBufferUsage usage, EUniformBufferValidation validation) override;
        void RHIUpdateUniformBuffer(FRHICommandListBase& commandList, FRHIUniformBuffer* buffer, const void* contents) override;
        FRHIBufferInitializer RHICreateBufferInitializer(FRHICommandListBase& commandList, const FRHIBufferCreateDesc& description) override;
        FShaderResourceViewRHIRef RHICreateShaderResourceView(FRHICommandListBase& commandList, FRHIViewableResource* resource, const FRHIViewDesc& description) override;
        FUnorderedAccessViewRHIRef RHICreateUnorderedAccessView(FRHICommandListBase& commandList, FRHIViewableResource* resource, const FRHIViewDesc& description) override;
        void RHICreateTransition(FRHITransition* transition, const FRHITransitionCreateInfo& info) override;
        void RHIReleaseTransition(FRHITransition* transition) override;
        FRHIFinalizeContextsResult RHIFinalizeContexts(FRHIFinalizeContextsArgs&& args) override;
        void RHISubmitCommandLists(FRHISubmitCommandListsArgs&& args) override;
        void RHIProcessDeleteQueue() override;
        void RHIBlockUntilGPUIdle() override;
        FGPUFenceRHIRef RHICreateGPUFence(const FName& name) override;

    protected:
        DynamicRHI(Shader::AgcTarget target, std::uint32_t addressHigh);
        void ConfigureTransitionStorage();

    private:
        Shader::AgcTarget target;
        std::uint32_t addressHigh;
        std::vector<std::unique_ptr<CommandList>> pendingCommandLists;
        bool submissionFailed = false;
    };
}

#endif
