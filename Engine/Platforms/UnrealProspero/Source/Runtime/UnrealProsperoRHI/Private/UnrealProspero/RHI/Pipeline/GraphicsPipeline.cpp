#include "UnrealProspero/RHI/Pipeline/GraphicsPipeline.h"
#include "UnrealProspero/RHI/Command/ColorBlend.h"
#include "UnrealProspero/RHI/Command/DepthStencil.h"
#include "UnrealProspero/RHI/Command/DepthBias.h"
#include "UnrealProspero/RHI/Command/Rasterization.h"
#include "UnrealProspero/RHI/Command/SampleCoverage.h"
#include "prx/libSceAgc/DcbState/include/Registers.hpp"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace UnrealProspero::RHI
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
        if (stages.GetMeshShader() || stages.GetAmplificationShader() || stages.GetWorkGraphShader()) throw std::runtime_error("Graphics pipeline shader stage is not implemented");
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
        shaders = std::make_unique<GraphicsShaders>(vertex, static_cast<PixelShader*>(stages.PixelShaderRHI), static_cast<GeometryShader*>(stages.GetGeometryShader()), primitiveType);
        if (!declaration && shaders->GetVertexInputLayout().GetVertexBufferUsageMask() != 0) throw std::invalid_argument("Vertex inputs require a vertex declaration");
    }

    FRHIGraphicsShader* GraphicsPipeline::GetShader(EShaderFrequency frequency) const
    {
        switch (frequency)
        {
        case SF_Vertex: return &shaders->GetVertexShader();
        case SF_Pixel: return shaders->GetPixelShader();
        case SF_Geometry: return shaders->GetGeometryShader();
        case SF_Mesh:
        case SF_Amplification: return nullptr;
        default: throw std::invalid_argument("Requested shader frequency is not a graphics stage");
        }
    }

    const FGraphicsPipelineStateInitializer& GraphicsPipeline::GetDescription() const { return description; }
    const GraphicsShaders& GraphicsPipeline::GetShaders() const { return *shaders; }
    const VertexDeclaration* GraphicsPipeline::GetVertexDeclaration() const { return declaration.GetReference(); }
    std::uint32_t GraphicsPipeline::GetNativePrimitiveType() const { return primitiveType; }

    std::uint32_t GraphicsPipeline::GetVertexCount(std::uint32_t primitiveCount) const
    {
        if (primitiveCount == 0) return 0;
        std::uint64_t count = primitiveCount;
        switch (description.PrimitiveType)
        {
        case PT_PointList: break;
        case PT_LineList: count *= 2; break;
        case PT_TriangleList:
        case PT_RectList: count *= 3; break;
        case PT_TriangleStrip: count += 2; break;
        default: throw std::invalid_argument("Invalid graphics primitive type");
        }
        if (count > std::numeric_limits<std::uint32_t>::max()) throw std::overflow_error("Draw vertex count exceeds the hardware field");
        return static_cast<std::uint32_t>(count);
    }

    void GraphicsPipeline::RecordColorState(CommandBuffer& commands, uint32 stencilReference, const FLinearColor& blendFactor) const
    {
        if (description.DepthStencilTargetFormat != PF_Unknown && description.DepthStencilTargetFormat != PF_ShadowDepth && description.DepthStencilTargetFormat != PF_DepthStencil) throw std::runtime_error("Graphics depth format is not implemented");
        const bool hasDepth = description.DepthStencilTargetFormat != PF_Unknown && description.DepthStencilAccess.IsUsingDepth();
        const bool hasStencil = description.DepthStencilTargetFormat == PF_DepthStencil && description.DepthStencilAccess.IsUsingStencil();
        if (description.DepthStencilAccess.IsUsingStencil() && description.DepthStencilTargetFormat != PF_DepthStencil) throw std::invalid_argument("Graphics pipeline has no stencil plane");
        const auto& stencil = depthStencil->GetDescription();
        const bool writesStencil = stencil.StencilWriteMask != 0 && ((stencil.bEnableFrontFaceStencil && (stencil.FrontFaceStencilFailStencilOp != SO_Keep || stencil.FrontFacePassStencilOp != SO_Keep || stencil.FrontFaceDepthFailStencilOp != SO_Keep)) || (stencil.bEnableBackFaceStencil && (stencil.BackFaceStencilFailStencilOp != SO_Keep || stencil.BackFacePassStencilOp != SO_Keep || stencil.BackFaceDepthFailStencilOp != SO_Keep)));
        if (hasStencil && writesStencil && !description.DepthStencilAccess.IsStencilWrite()) throw std::invalid_argument("Stencil write is enabled on a read-only attachment");
        if (hasDepth && depthStencil->GetDescription().bEnableDepthWrite && !description.DepthStencilAccess.IsDepthWrite()) throw std::invalid_argument("Depth write is enabled on a read-only attachment");
        const auto& raster = rasterizer->GetDescription();
        const bool depthBias = hasDepth && (raster.DepthBias != 0 || raster.SlopeScaleDepthBias != 0);
        if (blend->GetDescription().bUseAlphaToCoverage) throw std::runtime_error("Alpha to coverage is not implemented");
        for (uint32 index = 0; index < description.RenderTargetsEnabled; ++index)
        {
            const auto& target = blend->GetRenderTarget(index);
            for (const auto factor : {target.ColorSrcBlend, target.ColorDestBlend, target.AlphaSrcBlend, target.AlphaDestBlend})
            {
                if (factor == BF_Source1Color || factor == BF_InverseSource1Color || factor == BF_Source1Alpha || factor == BF_InverseSource1Alpha) throw std::runtime_error("Dual source blending is not implemented");
            }
        }
        const std::array factors{blendFactor.R, blendFactor.G, blendFactor.B, blendFactor.A};
        for (const auto factor : factors)
        {
            if (!std::isfinite(factor)) throw std::invalid_argument("Blend factor must be finite");
        }
        WriteSampleCoverage(commands, description.NumSamples);
        WriteColorBlendRegisters(commands, *blend, description.RenderTargetsEnabled);
        WriteDepthStencilRegisters(commands, *depthStencil, stencilReference, hasDepth, hasStencil, description.bDepthBounds);
        if (hasDepth) WriteDepthBias(commands, raster.DepthBias, raster.SlopeScaleDepthBias);
        WriteRasterizationRegisters(commands, *rasterizer, 0x240, 0x1080000, depthBias);
        const std::array registers{
            ShaderRegister{0x202, description.RenderTargetsEnabled ? 0xcc0011u : 1u},
            ShaderRegister{0x206, 0x43f}, ShaderRegister{0x292, 0x22}, ShaderRegister{0x2f9, 0x2d},
            ShaderRegister{0x2fa, 0x3f800000}, ShaderRegister{0x2fb, 0x3f800000},
            ShaderRegister{0x2fc, 0x3f800000}, ShaderRegister{0x2fd, 0x3f800000}
        };
        for (const auto value : registers) sceAgcDcbSetCxRegisterDirect(&commands, value);
        for (uint32 index = 0; index < factors.size(); ++index) sceAgcDcbSetCxRegisterDirect(&commands, {0x105 + index, std::bit_cast<uint32>(factors[index])});
    }
}
