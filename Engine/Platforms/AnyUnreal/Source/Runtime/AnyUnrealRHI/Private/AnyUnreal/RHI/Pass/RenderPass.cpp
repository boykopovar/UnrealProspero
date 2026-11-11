#include "AnyUnreal/RHI/Pass/RenderPass.h"
#include "AnyUnreal/RHI/Command/ColorTarget.h"
#include "prx/libSceAgc/DcbState/include/Registers.hpp"
#include <algorithm>
#include <stdexcept>

namespace AnyUnreal::RHI
{
    RenderPass::RenderPass(const FRHIRenderPassInfo& info, std::uint32_t addressHigh) : barrier(addressHigh)
    {
        if (info.DepthStencilRenderTarget.DepthStencilTarget || info.DepthStencilRenderTarget.ResolveTarget) throw std::runtime_error("Depth stencil render passes are not implemented");
        if (info.ShadingRateTexture || info.MultiViewCount != 0 || info.SubpassHint != ESubpassHint::None) throw std::runtime_error("Render pass shading rate, multiview and subpasses are not implemented");
        if (info.bOcclusionQueries || info.NumOcclusionQueries != 0) throw std::runtime_error("Render pass occlusion queries are not implemented");
        for (uint32 index = 0; index < targets.size(); ++index)
        {
            const auto& entry = info.ColorRenderTargets[index];
            if (entry.ResolveTarget) throw std::runtime_error("Render pass resolves are not implemented");
            if (!entry.RenderTarget) continue;
            if (entry.MipIndex != 0 || (entry.ArraySlice != -1 && entry.ArraySlice != 0)) throw std::out_of_range("Color render pass subresource does not exist");
            switch (GetLoadAction(entry.Action))
            {
            case ERenderTargetLoadAction::ENoAction:
            case ERenderTargetLoadAction::ELoad: break;
            case ERenderTargetLoadAction::EClear: throw std::runtime_error("Render pass color clears are not implemented");
            default: throw std::invalid_argument("Invalid render pass load action");
            }
            switch (GetStoreAction(entry.Action))
            {
            case ERenderTargetStoreAction::ENoAction:
            case ERenderTargetStoreAction::EStore: break;
            case ERenderTargetStoreAction::EMultisampleResolve: throw std::runtime_error("Render pass resolves are not implemented");
            default: throw std::invalid_argument("Invalid render pass store action");
            }
            auto& texture = ResolveTextureResource(entry.RenderTarget);
            const auto& description = texture.GetDesc();
            if (!EnumHasAnyFlags(description.Flags, ETextureCreateFlags::RenderTargetable)) throw std::invalid_argument("Render pass texture is not render targetable");
            targets[index] = texture.Capture();
            for (uint32 previous = 0; previous < index; ++previous)
            {
                if (targets[previous] == targets[index]) throw std::invalid_argument("Render pass binds the same color subresource more than once");
            }
            formats[index] = description.Format;
            srgb[index] = EnumHasAnyFlags(description.Flags, ETextureCreateFlags::SRGB);
            targetCount = index + 1;
            width = std::min(width, targets[index]->GetWidth());
            height = std::min(height, targets[index]->GetHeight());
        }
        if (targetCount == 0) throw std::runtime_error("Attachmentless render passes are not implemented");
    }

    std::vector<GpuMemoryRange> RenderPass::getMemoryRanges() const
    {
        std::vector<GpuMemoryRange> result;
        for (const auto& target : targets)
        {
            if (!target) continue;
            const auto& storage = target->GetStorage();
            result.push_back({storage.Data(), storage.Size()});
        }
        return result;
    }

    void RenderPass::Begin(CommandBuffer& commands)
    {
        if (begun || ended) throw std::runtime_error("Render pass has already begun");
        begun = true;
        WriteMemoryAcquire(commands, getMemoryRanges());
        for (uint32 index = 0; index < targets.size(); ++index)
        {
            if (targets[index]) WriteColorTarget(commands, index, *targets[index]);
            else DisableColorTarget(commands, index);
        }
        sceAgcDcbSetCxRegisterDirect(&commands, {0xc, 0});
        sceAgcDcbSetCxRegisterDirect(&commands, {0xd, width | (height << 16)});
        sceAgcDcbSetCxRegisterDirect(&commands, {0x200, 0});
    }

    void RenderPass::End(CommandBuffer& commands)
    {
        if (!begun || ended) throw std::runtime_error("Render pass is not active");
        ended = true;
        barrier.Release(commands);
        barrier.Acquire(commands, getMemoryRanges());
    }

    void RenderPass::ValidatePipeline(const FGraphicsPipelineStateInitializer& pipeline) const
    {
        if (!begun || ended) throw std::runtime_error("Draw requires an active render pass");
        if (pipeline.RenderTargetsEnabled != targetCount || pipeline.NumSamples != 1 || pipeline.DepthStencilTargetFormat != PF_Unknown) throw std::invalid_argument("Graphics pipeline attachments do not match the render pass");
        for (uint32 index = 0; index < targetCount; ++index)
        {
            if (pipeline.RenderTargetFormats[index] != formats[index]) throw std::invalid_argument("Graphics pipeline color format does not match the render pass");
            if (targets[index] && EnumHasAnyFlags(pipeline.RenderTargetFlags[index], ETextureCreateFlags::SRGB) != srgb[index]) throw std::invalid_argument("Graphics pipeline color encoding does not match the render pass");
        }
    }
}
