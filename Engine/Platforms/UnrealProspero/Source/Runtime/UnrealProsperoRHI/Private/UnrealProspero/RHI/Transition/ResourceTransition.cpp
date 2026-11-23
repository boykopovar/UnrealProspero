#include "UnrealProspero/RHI/Transition/ResourceTransition.h"
#include "UnrealProspero/RHI/Buffer/BufferView.h"
#include "UnrealProspero/RHI/Texture/TextureResource.h"
#include "UnrealProspero/RHI/Texture/TextureView.h"
#include <stdexcept>
#include <string>

namespace UnrealProspero::RHI
{
    ResourceTransition::ResourceTransition(const FRHITransitionCreateInfo& info, std::uint32_t addressHigh)
    {
        if (info.SrcPipelines != ERHIPipeline::Graphics || info.DstPipelines != ERHIPipeline::Graphics) throw std::runtime_error("Cross-pipeline resource transitions are not implemented");
        constexpr auto supportedFlags = ERHITransitionCreateFlags::NoSplit | ERHITransitionCreateFlags::NoFence | ERHITransitionCreateFlags::AllowDuringRenderPass | ERHITransitionCreateFlags::AllowDecayPipelines;
        if (EnumHasAnyFlags(info.Flags, ~supportedFlags)) throw std::runtime_error("Resource transition flags are not implemented: " + std::to_string(static_cast<unsigned>(info.Flags & ~supportedFlags)));
        if (!info.AliasingInfos.IsEmpty()) throw std::runtime_error("Transient resource aliasing is not implemented: " + std::to_string(info.AliasingInfos.Num()));
        for (const auto& entry : info.TransitionInfos)
        {
            if (!entry.Resource) throw std::invalid_argument("Resource transition requires a resource");
            if (EnumHasAnyFlags(entry.Flags, ~EResourceTransitionFlags::Mask) || entry.CommitInfo.IsSet()) throw std::runtime_error("Resource transition flags or reserved memory commits are not implemented");
            auto* retainedResource = entry.Resource;
            if (entry.Type == FRHITransitionInfo::EType::Buffer)
            {
                if (!entry.IsAllMips() || !entry.IsAllArraySlices() || !entry.IsAllPlaneSlices()) throw std::invalid_argument("Buffer transitions cannot specify texture subresources");
                if (entry.Resource->GetType() != RRT_Buffer) throw std::invalid_argument("Buffer transition has the wrong resource type");
            }
            else if (entry.Type == FRHITransitionInfo::EType::Texture)
            {
                auto& texture = ResolveTextureResource(entry.Resource);
                const auto& description = texture.GetDesc();
                const auto layers = description.ArraySize * (description.IsTextureCube() ? 6u : 1u);
                const auto planes = description.Format == PF_DepthStencil ? 2u : 1u;
                if ((!entry.IsAllMips() && entry.MipIndex >= description.NumMips) || (!entry.IsAllArraySlices() && entry.ArraySlice >= layers) || (!entry.IsAllPlaneSlices() && entry.PlaneSlice >= planes)) throw std::out_of_range("Texture transition selects a nonexistent subresource");
                retainedResource = &texture;
            }
            else if (entry.Type == FRHITransitionInfo::EType::UAV)
            {
                if (!entry.IsAllMips() || !entry.IsAllArraySlices() || !entry.IsAllPlaneSlices()) throw std::invalid_argument("UAV transitions select the complete view");
                if (entry.Resource->GetType() != RRT_UnorderedAccessView) throw std::invalid_argument("UAV transition has the wrong resource type");
            }
            else throw std::runtime_error("Resource transition type is not implemented");
            resources.emplace_back(retainedResource);
        }
        barrier = std::make_unique<MemoryBarrier>(addressHigh);
    }

    void ResourceTransition::Begin(CommandBuffer& commands, ERHIPipeline pipeline)
    {
        if (pipeline != ERHIPipeline::Graphics) throw std::invalid_argument("Resource transition must begin on the graphics pipeline");
        if (begun) throw std::runtime_error("Resource transition has already begun");
        for (const auto& resource : resources)
        {
            const BufferAllocation* allocation = nullptr;
            if (resource->GetType() == RRT_Buffer)
            {
                auto storage = static_cast<Buffer*>(resource.GetReference())->Capture();
                allocation = storage.get();
                allocations.push_back(std::move(storage));
            }
            else if (resource->GetType() == RRT_Texture) allocation = &static_cast<TextureResource*>(resource.GetReference())->GetStorage();
            else
            {
                auto* view = static_cast<FRHIUnorderedAccessView*>(resource.GetReference());
                if (view->IsBuffer())
                {
                    auto storage = static_cast<BufferUnorderedAccessView*>(view)->Capture().CaptureStorage();
                    allocation = storage.get();
                    allocations.push_back(std::move(storage));
                }
                else allocation = static_cast<TextureUnorderedAccessView*>(view)->GetStorage();
            }
            if (!allocation) throw std::invalid_argument("Resource transition requires a nonempty allocation");
            ranges.push_back({allocation->Data(), allocation->Size()});
        }
        begun = true;
        barrier->Release(commands);
    }

    void ResourceTransition::End(CommandBuffer& commands, ERHIPipeline pipeline)
    {
        if (pipeline != ERHIPipeline::Graphics) throw std::invalid_argument("Resource transition must end on the graphics pipeline");
        if (!begun || ended) throw std::runtime_error("Resource transition must end once after it begins");
        ended = true;
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
