#ifndef UNREALPROSPERO_RHI_RENDER_PASS_H
#define UNREALPROSPERO_RHI_RENDER_PASS_H

#include "UnrealProspero/RHI/Texture/TextureResource.h"
#include "UnrealProspero/RHI/Command/MemoryBarrier.h"
#include <optional>

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API RenderPass final
    {
    public:
        RenderPass(const FRHIRenderPassInfo& info, std::uint32_t addressHigh);
        void Begin(CommandBuffer& commands);
        void End(CommandBuffer& commands);
        void ValidatePipeline(const FGraphicsPipelineStateInitializer& pipeline) const;

    private:
        std::vector<GpuMemoryRange> getMemoryRanges() const;
        std::array<std::shared_ptr<const Texture2D>, MaxSimultaneousRenderTargets> targets{};
        std::array<std::shared_ptr<const LinearTexture3D>, MaxSimultaneousRenderTargets> volumes{};
        std::array<EPixelFormat, MaxSimultaneousRenderTargets> formats{};
        std::array<bool, MaxSimultaneousRenderTargets> srgb{};
        std::array<std::uint32_t, MaxSimultaneousRenderTargets> mips{};
        std::array<std::uint32_t, MaxSimultaneousRenderTargets> layers{};
        std::array<std::uint32_t, MaxSimultaneousRenderTargets> layerCounts{};
        std::array<std::optional<std::array<float, 4>>, MaxSimultaneousRenderTargets> clearColors{};
        std::shared_ptr<const DepthTexture2D> depthTarget;
        std::optional<float> clearDepth;
        FExclusiveDepthStencil depthAccess;
        MemoryBarrier barrier;
        uint32 targetCount = 0;
        uint32 width = 16384;
        uint32 height = 16384;
        bool begun = false;
        bool ended = false;
    };
}

#endif
