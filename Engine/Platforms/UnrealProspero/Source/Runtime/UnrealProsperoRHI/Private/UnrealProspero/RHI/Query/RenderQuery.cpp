#include "UnrealProspero/RHI/Query/RenderQuery.h"
#include "Misc/ScopeLock.h"
#include <stdexcept>

namespace UnrealProspero::RHI
{
    RenderQuery::RenderQuery(ERenderQueryType type)
    {
        if (type != RQT_Occlusion) throw std::runtime_error("Render query type is not implemented");
    }

    std::shared_ptr<OcclusionSnapshot> RenderQuery::Begin(CommandBuffer& commands, std::uint32_t addressHigh)
    {
        FScopeLock lock(&mutex);
        if (snapshot && !snapshot->IsEnded()) throw std::runtime_error("Render query is already active");
        auto next = std::make_shared<OcclusionSnapshot>(addressHigh);
        next->Begin(commands);
        snapshot = std::move(next);
        return snapshot;
    }

    std::shared_ptr<OcclusionSnapshot> RenderQuery::End(CommandBuffer& commands)
    {
        FScopeLock lock(&mutex);
        if (!snapshot) throw std::runtime_error("Render query has not begun");
        snapshot->End(commands);
        return snapshot;
    }

    bool RenderQuery::TryGetResult(uint64& result, bool wait) const
    {
        std::shared_ptr<OcclusionSnapshot> current;
        {
            FScopeLock lock(&mutex);
            if (!snapshot || !snapshot->IsEnded()) return false;
            current = snapshot;
        }
        std::uint64_t value = 0;
        if (!current->TryGetResult(value, wait)) return false;
        result = value;
        return true;
    }
}
