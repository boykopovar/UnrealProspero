#ifndef ANYUNREAL_RHI_OCCLUSION_SNAPSHOT_H
#define ANYUNREAL_RHI_OCCLUSION_SNAPSHOT_H

#include "AnyUnreal/RHI/Command/Completion.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API OcclusionSnapshot final
    {
    public:
        explicit OcclusionSnapshot(std::uint32_t addressHigh);
        void Begin(CommandBuffer& commands);
        void End(CommandBuffer& commands);
        bool TryGetResult(std::uint64_t& result, bool wait) const;
        bool IsEnded() const;

    private:
        BufferAllocation counters;
        Completion completion;
        bool begun = false;
        bool ended = false;
    };
}

#endif
