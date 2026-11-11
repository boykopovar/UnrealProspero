#ifndef ANYUNREAL_RHI_BUFFER_TRANSITION_H
#define ANYUNREAL_RHI_BUFFER_TRANSITION_H

#include "AnyUnreal/RHI/Command/MemoryBarrier.h"
#include "RHIResources.h"
#include <memory>
#include <vector>

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API BufferTransition final
    {
    public:
        BufferTransition(const FRHITransitionCreateInfo& info, std::uint32_t addressHigh);
        void Begin(CommandBuffer& commands, ERHIPipeline pipeline);
        void End(CommandBuffer& commands, ERHIPipeline pipeline);

    private:
        std::vector<TRefCountPtr<FRHIResource>> resources;
        std::vector<std::shared_ptr<BufferAllocation>> allocations;
        std::vector<GpuMemoryRange> ranges;
        std::unique_ptr<MemoryBarrier> barrier;
    };

    struct TransitionData
    {
        std::shared_ptr<BufferTransition> State;
    };

    ANYUNREALRHI_API void InitializeBufferTransition(FRHITransition& transition, const FRHITransitionCreateInfo& info, std::uint32_t addressHigh);
    ANYUNREALRHI_API void DestroyBufferTransition(FRHITransition& transition);
}

#endif
