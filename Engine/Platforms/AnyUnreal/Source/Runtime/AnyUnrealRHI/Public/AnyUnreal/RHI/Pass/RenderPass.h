#ifndef ANYUNREAL_RHI_RENDER_PASS_H
#define ANYUNREAL_RHI_RENDER_PASS_H

#include "AnyUnreal/RHI/Texture/TextureResource.h"
#include "AnyUnreal/RHI/Command/MemoryBarrier.h"
#include <optional>

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API RenderPass final
    {
    public:
        RenderPass(const FRHIRenderPassInfo& info, std::uint32_t addressHigh);
        void Begin(CommandBuffer& commands);
        void End(CommandBuffer& commands);
        void ValidatePipeline(const FGraphicsPipelineStateInitializer& pipeline) const;

    private:
        std::vector<GpuMemoryRange> getMemoryRanges() const;
        std::array<std::shared_ptr<const LinearTexture2D>, MaxSimultaneousRenderTargets> targets{};
        std::array<EPixelFormat, MaxSimultaneousRenderTargets> formats{};
        std::array<bool, MaxSimultaneousRenderTargets> srgb{};
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
