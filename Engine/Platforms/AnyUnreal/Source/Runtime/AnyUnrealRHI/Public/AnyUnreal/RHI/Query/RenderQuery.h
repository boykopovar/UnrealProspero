#ifndef ANYUNREAL_RHI_RENDER_QUERY_H
#define ANYUNREAL_RHI_RENDER_QUERY_H

#include "AnyUnreal/RHI/Query/OcclusionSnapshot.h"
#include "RHIResources.h"
#include "HAL/CriticalSection.h"
#include <memory>

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API RenderQuery final : public FRHIRenderQuery
    {
    public:
        explicit RenderQuery(ERenderQueryType type);
        std::shared_ptr<OcclusionSnapshot> Begin(CommandBuffer& commands, std::uint32_t addressHigh);
        std::shared_ptr<OcclusionSnapshot> End(CommandBuffer& commands);
        bool TryGetResult(uint64& result, bool wait) const;

    private:
        mutable FCriticalSection mutex;
        std::shared_ptr<OcclusionSnapshot> snapshot;
    };
}

#endif
