#include "AnyUnreal/RHI/Transition/BufferTransition.h"
#include "AnyUnreal/RHI/Buffer/BufferView.h"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    BufferTransition::BufferTransition(const FRHITransitionCreateInfo& info, std::uint32_t addressHigh)
    {
        if (info.SrcPipelines != ERHIPipeline::Graphics || info.DstPipelines != ERHIPipeline::Graphics) throw std::runtime_error("Cross-pipeline resource transitions are not implemented");
        if (EnumHasAnyFlags(info.Flags, ~ERHITransitionCreateFlags::NoSplit) || !info.AliasingInfos.IsEmpty()) throw std::runtime_error("Resource transition flags or aliasing are not implemented");
        for (const auto& entry : info.TransitionInfos)
        {
            if (!entry.Resource) throw std::invalid_argument("Resource transition requires a resource");
            if (!entry.IsAllMips() || !entry.IsAllArraySlices() || !entry.IsAllPlaneSlices()) throw std::invalid_argument("Buffer transitions cannot specify texture subresources");
            if (entry.Flags != EResourceTransitionFlags::None || entry.CommitInfo.IsSet()) throw std::runtime_error("Resource transition flags or reserved memory commits are not implemented");
            const BufferAllocation* allocation = nullptr;
            if (entry.Type == FRHITransitionInfo::EType::Buffer)
            {
                if (entry.Resource->GetType() != RRT_Buffer) throw std::invalid_argument("Buffer transition has the wrong resource type");
                auto storage = static_cast<Buffer*>(entry.Buffer)->Capture();
                allocation = storage.get();
                allocations.push_back(std::move(storage));
            }
            else if (entry.Type == FRHITransitionInfo::EType::UAV)
            {
                if (entry.Resource->GetType() != RRT_UnorderedAccessView) throw std::invalid_argument("UAV transition has the wrong resource type");
                if (!entry.UAV->IsBuffer()) throw std::runtime_error("Texture transitions are not implemented");
                allocation = static_cast<BufferUnorderedAccessView*>(entry.UAV)->GetStorage();
                if (!allocation) throw std::invalid_argument("Resource transition requires a nonempty buffer view");
            }
            else throw std::runtime_error("Resource transition type is not implemented");
            resources.emplace_back(entry.Resource);
            ranges.push_back({allocation->Data(), allocation->Size()});
        }
        barrier = std::make_unique<MemoryBarrier>(addressHigh);
    }

    void BufferTransition::Begin(CommandBuffer& commands, ERHIPipeline pipeline)
    {
        if (pipeline != ERHIPipeline::Graphics) throw std::invalid_argument("Buffer transition must begin on the graphics pipeline");
        barrier->Release(commands);
    }

    void BufferTransition::End(CommandBuffer& commands, ERHIPipeline pipeline)
    {
        if (pipeline != ERHIPipeline::Graphics) throw std::invalid_argument("Buffer transition must end on the graphics pipeline");
        barrier->Acquire(commands, ranges);
    }

    void InitializeBufferTransition(FRHITransition& transition, const FRHITransitionCreateInfo& info, std::uint32_t addressHigh)
    {
        auto state = std::make_shared<BufferTransition>(info, addressHigh);
        std::construct_at(transition.GetPrivateData<TransitionData>(), TransitionData{std::move(state)});
    }

    void DestroyBufferTransition(FRHITransition& transition)
    {
        std::destroy_at(transition.GetPrivateData<TransitionData>());
    }
}
