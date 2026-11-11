#include "AnyUnreal/RHI/Pipeline/GraphicsPipeline.h"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    namespace
    {
        std::uint32_t convertPrimitiveType(EPrimitiveType type)
        {
            switch (type)
            {
            case PT_PointList: return 1;
            case PT_LineList: return 2;
            case PT_TriangleList: return 4;
            case PT_TriangleStrip: return 6;
            case PT_RectList: return 7;
            case PT_QuadList: throw std::runtime_error("Quad topology is not implemented");
            default: throw std::invalid_argument("Invalid graphics primitive type");
            }
        }
    }

    GraphicsPipeline::GraphicsPipeline(const FGraphicsPipelineStateInitializer& initializer) : description(initializer), primitiveType(convertPrimitiveType(initializer.PrimitiveType))
    {
        const auto& stages = initializer.BoundShaderState;
        if (stages.GetMeshShader() || stages.GetAmplificationShader() || stages.GetGeometryShader() || stages.GetWorkGraphShader()) throw std::runtime_error("Graphics pipeline shader stage is not implemented");
        if (!stages.VertexShaderRHI || !initializer.BlendState || !initializer.DepthStencilState || !initializer.RasterizerState) throw std::invalid_argument("Graphics pipeline requires a vertex shader and fixed function states");
        if (initializer.RenderTargetsEnabled > MaxSimultaneousRenderTargets) throw std::out_of_range("Graphics pipeline exceeds the render target limit");
        if (initializer.NumSamples != 1) throw std::runtime_error("Multisample graphics pipelines are not implemented");
        if (initializer.MultiViewCount != 0 || initializer.bHasFragmentDensityAttachment) throw std::runtime_error("Multiview and fragment density pipelines are not implemented");
        if (initializer.SubpassHint != ESubpassHint::None || initializer.SubpassIndex != 0) throw std::runtime_error("Graphics subpass pipelines are not implemented");
        if (initializer.ConservativeRasterization != EConservativeRasterization::Disabled) throw std::runtime_error("Conservative rasterization is not implemented");
        if (initializer.ShadingRate != EVRSShadingRate::VRSSR_1x1) throw std::runtime_error("Variable shading rate is not implemented");
        for (uint32 index = 0; index < initializer.RenderTargetsEnabled; ++index)
        {
            if (initializer.RenderTargetFormats[index] >= PF_MAX) throw std::invalid_argument("Invalid graphics render target format");
        }
        if (initializer.DepthStencilTargetFormat < PF_Unknown || initializer.DepthStencilTargetFormat >= PF_MAX) throw std::invalid_argument("Invalid graphics depth stencil format");
        declaration = static_cast<VertexDeclaration*>(stages.VertexDeclarationRHI);
        blend = static_cast<BlendState*>(initializer.BlendState);
        depthStencil = static_cast<DepthStencilState*>(initializer.DepthStencilState);
        rasterizer = static_cast<RasterizerState*>(initializer.RasterizerState);
        auto& vertex = *static_cast<VertexShader*>(stages.VertexShaderRHI);
        if (!declaration && vertex.GetCode().GetUserData().GetVertexBufferUsageMask() != 0) throw std::invalid_argument("Vertex inputs require a vertex declaration");
        shaders = std::make_unique<GraphicsShaders>(vertex, static_cast<PixelShader*>(stages.PixelShaderRHI), primitiveType);
    }

    FRHIGraphicsShader* GraphicsPipeline::GetShader(EShaderFrequency frequency) const
    {
        switch (frequency)
        {
        case SF_Vertex: return &shaders->GetVertexShader();
        case SF_Pixel: return shaders->GetPixelShader();
        case SF_Geometry:
        case SF_Mesh:
        case SF_Amplification: return nullptr;
        default: throw std::invalid_argument("Requested shader frequency is not a graphics stage");
        }
    }

    const FGraphicsPipelineStateInitializer& GraphicsPipeline::GetDescription() const { return description; }
    const GraphicsShaders& GraphicsPipeline::GetShaders() const { return *shaders; }
    const VertexDeclaration* GraphicsPipeline::GetVertexDeclaration() const { return declaration.GetReference(); }
    std::uint32_t GraphicsPipeline::GetNativePrimitiveType() const { return primitiveType; }
}
