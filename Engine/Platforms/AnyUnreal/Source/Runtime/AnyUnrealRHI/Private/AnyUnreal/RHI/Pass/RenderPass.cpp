#include "AnyUnreal/RHI/Pass/RenderPass.h"
#include "AnyUnreal/RHI/Command/ColorTarget.h"
#include "AnyUnreal/RHI/Command/ColorClear.h"
#include "AnyUnreal/RHI/Command/DepthTarget.h"
#include "prx/libSceAgc/DcbState/include/Registers.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace AnyUnreal::RHI
{
    RenderPass::RenderPass(const FRHIRenderPassInfo& info, std::uint32_t addressHigh) : barrier(addressHigh)
    {
        const auto& depth = info.DepthStencilRenderTarget;
        if (depth.ResolveTarget) throw std::runtime_error("Depth resolve is not implemented");
        if (depth.DepthStencilTarget)
        {
            auto& texture = ResolveTextureResource(depth.DepthStencilTarget);
            const auto& description = texture.GetDesc();
            if (description.Format != PF_ShadowDepth || !EnumHasAnyFlags(description.Flags, ETextureCreateFlags::DepthStencilTargetable)) throw std::invalid_argument("Render pass requires a D32 depth target");
            depthAccess = depth.ExclusiveDepthStencil;
            if (depthAccess.IsUsingStencil()) throw std::invalid_argument("D32 render pass has no stencil plane");
            const auto action = GetDepthActions(depth.Action);
            const auto stencilAction = GetStencilActions(depth.Action);
            if (GetLoadAction(stencilAction) != ERenderTargetLoadAction::ENoAction || GetStoreAction(stencilAction) != ERenderTargetStoreAction::ENoAction) throw std::invalid_argument("D32 render pass cannot load or store stencil");
            switch (GetLoadAction(action))
            {
            case ERenderTargetLoadAction::ENoAction:
            case ERenderTargetLoadAction::ELoad: break;
            case ERenderTargetLoadAction::EClear:
                if (!depthAccess.IsDepthWrite()) throw std::invalid_argument("Depth clear requires writable depth access");
                if (description.ClearValue.ColorBinding != EClearBinding::EDepthStencilBound) throw std::invalid_argument("Depth clear requires a depth clear binding");
                clearDepth = texture.GetDepthClearValue();
                if (!std::isfinite(*clearDepth) || *clearDepth < 0 || *clearDepth > 1) throw std::invalid_argument("Depth clear value must be finite and normalized");
                break;
            default: throw std::invalid_argument("Invalid depth load action");
            }
            switch (GetStoreAction(action))
            {
            case ERenderTargetStoreAction::ENoAction:
            case ERenderTargetStoreAction::EStore: break;
            default: throw std::runtime_error("Depth store action is not implemented");
            }
            depthTarget = texture.CaptureDepth();
            width = depthTarget->GetWidth();
            height = depthTarget->GetHeight();
        }
        if (info.ShadingRateTexture || info.MultiViewCount != 0 || info.SubpassHint != ESubpassHint::None) throw std::runtime_error("Render pass shading rate, multiview and subpasses are not implemented");
        if (info.NumOcclusionQueries < 0 || (!info.bOcclusionQueries && info.NumOcclusionQueries != 0)) throw std::invalid_argument("Render pass occlusion query count is inconsistent");
        for (uint32 index = 0; index < targets.size(); ++index)
        {
            const auto& entry = info.ColorRenderTargets[index];
            if (entry.ResolveTarget) throw std::runtime_error("Render pass resolves are not implemented");
            if (!entry.RenderTarget) continue;
            if (entry.ArraySlice != -1 && entry.ArraySlice != 0) throw std::out_of_range("Color render pass subresource does not exist");
            switch (GetLoadAction(entry.Action))
            {
            case ERenderTargetLoadAction::ENoAction:
            case ERenderTargetLoadAction::ELoad:
            case ERenderTargetLoadAction::EClear: break;
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
            const auto& mip = targets[index]->GetMipLayout(entry.MipIndex);
            mips[index] = entry.MipIndex;
            if (GetLoadAction(entry.Action) == ERenderTargetLoadAction::EClear)
            {
                if (description.ClearValue.ColorBinding != EClearBinding::EColorBound) throw std::invalid_argument("Color clear requires a color clear binding");
                const auto color = description.ClearValue.GetClearColor();
                clearColors[index] = std::array{color.R, color.G, color.B, color.A};
            }
            for (uint32 previous = 0; previous < index; ++previous)
            {
                if (targets[previous] == targets[index] && mips[previous] == mips[index]) throw std::invalid_argument("Render pass binds the same color subresource more than once");
            }
            formats[index] = description.Format;
            srgb[index] = EnumHasAnyFlags(description.Flags, ETextureCreateFlags::SRGB);
            targetCount = index + 1;
            width = std::min(width, mip.Width);
            height = std::min(height, mip.Height);
        }
        if (targetCount == 0 && !depthTarget) throw std::runtime_error("Attachmentless render passes are not implemented");
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
        if (depthTarget)
        {
            const auto& storage = depthTarget->GetStorage();
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
            if (targets[index])
            {
                if (clearColors[index]) WriteColorClear(commands, *targets[index], *clearColors[index], mips[index]);
                WriteColorTarget(commands, index, *targets[index], mips[index]);
            }
            else DisableColorTarget(commands, index);
        }
        if (depthTarget)
        {
            WriteDepthTarget(commands, *depthTarget);
            if (clearDepth) WriteDepthClear(commands, *depthTarget, *clearDepth);
        }
        else DisableDepthTarget(commands);
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
        if (pipeline.RenderTargetsEnabled != targetCount || pipeline.NumSamples != 1 || pipeline.DepthStencilTargetFormat != (depthTarget ? PF_ShadowDepth : PF_Unknown)) throw std::invalid_argument("Graphics pipeline attachments do not match the render pass");
        if (depthTarget && pipeline.DepthStencilAccess != depthAccess) throw std::invalid_argument("Pipeline depth access does not match the render pass");
        for (uint32 index = 0; index < targetCount; ++index)
        {
            if (pipeline.RenderTargetFormats[index] != formats[index]) throw std::invalid_argument("Graphics pipeline color format does not match the render pass");
            if (targets[index] && EnumHasAnyFlags(pipeline.RenderTargetFlags[index], ETextureCreateFlags::SRGB) != srgb[index]) throw std::invalid_argument("Graphics pipeline color encoding does not match the render pass");
        }
    }
}
