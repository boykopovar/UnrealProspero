#ifndef ANYUNREAL_RHI_GRAPHICS_PIPELINE_H
#define ANYUNREAL_RHI_GRAPHICS_PIPELINE_H

#include "AnyUnreal/RHI/Pipeline/GraphicsShaders.h"
#include "AnyUnreal/RHI/State/VertexDeclaration.h"
#include "AnyUnreal/RHI/State/BlendState.h"
#include "AnyUnreal/RHI/State/DepthStencilState.h"
#include "AnyUnreal/RHI/State/RasterizerState.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API GraphicsPipeline final : public FRHIGraphicsPipelineState
    {
    public:
        explicit GraphicsPipeline(const FGraphicsPipelineStateInitializer& initializer);
        FRHIGraphicsShader* GetShader(EShaderFrequency frequency) const override;
        const FGraphicsPipelineStateInitializer& GetDescription() const;
        const GraphicsShaders& GetShaders() const;
        const VertexDeclaration* GetVertexDeclaration() const;
        std::uint32_t GetNativePrimitiveType() const;
        std::uint32_t GetVertexCount(std::uint32_t primitiveCount) const;
        void RecordColorState(CommandBuffer& commands, uint32 stencilReference, const FLinearColor& blendFactor) const;

    private:
        FGraphicsPipelineStateInitializer description;
        TRefCountPtr<VertexDeclaration> declaration;
        TRefCountPtr<BlendState> blend;
        TRefCountPtr<DepthStencilState> depthStencil;
        TRefCountPtr<RasterizerState> rasterizer;
        std::unique_ptr<GraphicsShaders> shaders;
        std::uint32_t primitiveType;
    };
}

#endif
