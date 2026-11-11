#include "AnyUnreal/RHI/Transition/ResourceTransition.h"
#include "AnyUnreal/RHI/Buffer/BufferView.h"
#include "AnyUnreal/RHI/Texture/TextureResource.h"
#include "AnyUnreal/RHI/Texture/TextureView.h"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    ResourceTransition::ResourceTransition(const FRHITransitionCreateInfo& info, std::uint32_t addressHigh)
    {
        if (info.SrcPipelines != ERHIPipeline::Graphics || info.DstPipelines != ERHIPipeline::Graphics) throw std::runtime_error("Cross-pipeline resource transitions are not implemented");
        if (EnumHasAnyFlags(info.Flags, ~ERHITransitionCreateFlags::NoSplit) || !info.AliasingInfos.IsEmpty()) throw std::runtime_error("Resource transition flags or aliasing are not implemented");
        for (const auto& entry : info.TransitionInfos)
        {
            if (!entry.Resource) throw std::invalid_argument("Resource transition requires a resource");
            if (entry.Flags != EResourceTransitionFlags::None || entry.CommitInfo.IsSet()) throw std::runtime_error("Resource transition flags or reserved memory commits are not implemented");
            const BufferAllocation* allocation = nullptr;
            auto* retainedResource = entry.Resource;
            if (entry.Type == FRHITransitionInfo::EType::Buffer)
            {
                if (!entry.IsAllMips() || !entry.IsAllArraySlices() || !entry.IsAllPlaneSlices()) throw std::invalid_argument("Buffer transitions cannot specify texture subresources");
                if (entry.Resource->GetType() != RRT_Buffer) throw std::invalid_argument("Buffer transition has the wrong resource type");
                auto storage = static_cast<Buffer*>(entry.Buffer)->Capture();
                allocation = storage.get();
                allocations.push_back(std::move(storage));
            }
            else if (entry.Type == FRHITransitionInfo::EType::Texture)
            {
                auto& texture = ResolveTextureResource(entry.Resource);
                if ((!entry.IsAllMips() && entry.MipIndex != 0) || (!entry.IsAllArraySlices() && entry.ArraySlice != 0) || (!entry.IsAllPlaneSlices() && entry.PlaneSlice != 0)) throw std::out_of_range("Texture transition selects a nonexistent subresource");
                allocation = &texture.GetStorage();
                retainedResource = &texture;
            }
            else if (entry.Type == FRHITransitionInfo::EType::UAV)
            {
                if (!entry.IsAllMips() || !entry.IsAllArraySlices() || !entry.IsAllPlaneSlices()) throw std::invalid_argument("UAV transitions select the complete view");
                if (entry.Resource->GetType() != RRT_UnorderedAccessView) throw std::invalid_argument("UAV transition has the wrong resource type");
                if (entry.UAV->IsBuffer())
                {
                    auto storage = static_cast<BufferUnorderedAccessView*>(entry.UAV)->Capture().CaptureStorage();
                    allocation = storage.get();
                    allocations.push_back(std::move(storage));
                }
                else allocation = static_cast<TextureUnorderedAccessView*>(entry.UAV)->GetStorage();
                if (!allocation) throw std::invalid_argument("Resource transition requires a nonempty buffer view");
            }
            else throw std::runtime_error("Resource transition type is not implemented");
            resources.emplace_back(retainedResource);
            ranges.push_back({allocation->Data(), allocation->Size()});
        }
        barrier = std::make_unique<MemoryBarrier>(addressHigh);
    }

    void ResourceTransition::Begin(CommandBuffer& commands, ERHIPipeline pipeline)
    {
        if (pipeline != ERHIPipeline::Graphics) throw std::invalid_argument("Resource transition must begin on the graphics pipeline");
        barrier->Release(commands);
    }

    void ResourceTransition::End(CommandBuffer& commands, ERHIPipeline pipeline)
    {
        if (pipeline != ERHIPipeline::Graphics) throw std::invalid_argument("Resource transition must end on the graphics pipeline");
        barrier->Acquire(commands, ranges);
    }

    void InitializeResourceTransition(FRHITransition& transition, const FRHITransitionCreateInfo& info, std::uint32_t addressHigh)
    {
        auto state = std::make_shared<ResourceTransition>(info, addressHigh);
        std::construct_at(transition.GetPrivateData<TransitionData>(), TransitionData{std::move(state)});
    }

    void DestroyResourceTransition(FRHITransition& transition)
    {
        std::destroy_at(transition.GetPrivateData<TransitionData>());
    }
}
