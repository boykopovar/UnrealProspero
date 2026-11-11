#ifndef ANYUNREAL_RHI_RESOURCE_TRANSITION_H
#define ANYUNREAL_RHI_RESOURCE_TRANSITION_H

#include "AnyUnreal/RHI/Command/MemoryBarrier.h"
#include "RHIResources.h"
#include <memory>
#include <vector>

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API ResourceTransition final
    {
    public:
        ResourceTransition(const FRHITransitionCreateInfo& info, std::uint32_t addressHigh);
        void Begin(CommandBuffer& commands, ERHIPipeline pipeline);
        void End(CommandBuffer& commands, ERHIPipeline pipeline);

    private:
        std::vector<TRefCountPtr<FRHIResource>> resources;
        std::vector<std::shared_ptr<BufferAllocation>> allocations;
        std::vector<GpuMemoryRange> ranges;
        std::unique_ptr<MemoryBarrier> barrier;
        bool begun = false;
        bool ended = false;
    };

    struct TransitionData
    {
        std::shared_ptr<ResourceTransition> State;
    };

    ANYUNREALRHI_API void InitializeResourceTransition(FRHITransition& transition, const FRHITransitionCreateInfo& info, std::uint32_t addressHigh);
    ANYUNREALRHI_API void DestroyResourceTransition(FRHITransition& transition);
}

#endif
