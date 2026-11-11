#include "AnyUnreal/RHI/Context/GraphicsContext.h"
#include "AnyUnreal/RHI/Command/Viewport.h"
#include "AnyUnreal/RHI/Command/DepthBounds.h"
#include "AnyUnreal/RHI/Command/IndirectDraw.h"
#include "AnyUnreal/RHI/Command/TextureCopy.h"
#include "AnyUnreal/RHI/Texture/TextureResource.h"
#include "RHICommandList.h"
#include "RHI.h"
#include <stdexcept>
#include <limits>
#include <utility>
#include <cmath>
#include <algorithm>
#include "prx/libSceAgc/DcbDraw/include/IndexBuffer.hpp"
#include "prx/libSceAgc/DcbDraw/include/DrawIndexed.hpp"
#include "prx/libSceAgc/DcbDraw/include/Instancing.hpp"

namespace AnyUnreal::RHI
{
    void GraphicsContext::RHISetDepthBounds(float minDepth, float maxDepth)
    {
        RecordCommands([&](CommandBuffer& commands) { WriteDepthBounds(commands, minDepth, maxDepth); });
    }

    GraphicsContext::GraphicsContext(std::size_t initialCommandBytes, std::uint32_t addressHigh) : ComputeContext(initialCommandBytes, addressHigh)
    {
        RecordCommands([](CommandBuffer& commands) { WriteScissor(commands, {0, 0, 32767, 32767}); });
    }

    void GraphicsContext::RHICopyTexture(FRHITexture* source, FRHITexture* destination, const FRHICopyTextureInfo& info)
    {
        RecordCommands([&](CommandBuffer& commands)
        {
            if (activeRenderPass) throw std::runtime_error("Texture copy cannot execute inside a render pass");
            if (!source || !destination) throw std::invalid_argument("Texture copy requires source and destination textures");
            if (info.SourceMipIndex != 0 || info.DestMipIndex != 0 || info.NumMips != 1 || info.SourceSliceIndex != 0 || info.DestSliceIndex != 0 || info.NumSlices != 1) throw std::invalid_argument("Texture copy requires the available mip and slice");
            if (info.SourcePosition.X < 0 || info.SourcePosition.Y < 0 || info.SourcePosition.Z != 0 || info.DestPosition.X < 0 || info.DestPosition.Y < 0 || info.DestPosition.Z != 0) throw std::invalid_argument("Invalid two-dimensional texture copy position");
            auto sourceStorage = static_cast<TextureResource*>(source)->Capture();
            auto destinationStorage = static_cast<TextureResource*>(destination)->Capture();
            const bool whole = info.Size == FIntVector::ZeroValue;
            if (!whole && (info.Size.X <= 0 || info.Size.Y <= 0 || info.Size.Z < 0 || info.Size.Z > 1)) throw std::invalid_argument("Invalid two-dimensional texture copy size");
            const TextureCopyRegion region{static_cast<uint32>(info.SourcePosition.X), static_cast<uint32>(info.SourcePosition.Y), static_cast<uint32>(info.DestPosition.X), static_cast<uint32>(info.DestPosition.Y), whole ? sourceStorage->GetWidth() : static_cast<uint32>(info.Size.X), whole ? sourceStorage->GetHeight() : static_cast<uint32>(info.Size.Y)};
            copiedTextures.reserve(copiedTextures.size() + 2);
            copiedTextures.push_back(sourceStorage);
            copiedTextures.push_back(destinationStorage);
            WriteTextureCopy(commands, *destinationStorage, *sourceStorage, region);
        });
    }

    void GraphicsContext::UploadTexture(std::shared_ptr<const LinearTexture2D> destination, std::shared_ptr<TextureUpload> upload, uint32 x, uint32 y)
    {
        RecordCommands([&](CommandBuffer& commands)
        {
            if (activeRenderPass) throw std::runtime_error("Texture upload cannot execute inside a render pass");
            if (!destination || !upload) throw std::invalid_argument("Texture upload requires storage and data");
            copiedTextures.push_back(destination);
            textureUploads.push_back(upload);
            upload->Record(commands, *destination, x, y);
        });
    }

    void GraphicsContext::RHISetBlendFactor(const FLinearColor& factor)
    {
        RecordCommands([&](CommandBuffer&)
        {
            if (!std::isfinite(factor.R) || !std::isfinite(factor.G) || !std::isfinite(factor.B) || !std::isfinite(factor.A)) throw std::invalid_argument("Blend factor must be finite");
            blendFactor = factor;
        });
    }

    void GraphicsContext::RHIDrawPrimitive(uint32 baseVertexIndex, uint32 primitiveCount, uint32 instanceCount)
    {
        RecordCommands([&](CommandBuffer& commands)
        {
            if (!pipeline || !activeRenderPass) throw std::runtime_error("Draw requires a graphics pipeline and render pass");
            activeRenderPass->ValidatePipeline(pipeline->GetDescription());
            const auto vertexCount = pipeline->GetVertexCount(primitiveCount);
            if (vertexCount == 0) return;
            instanceCount = std::max(1u, instanceCount);
            if (baseVertexIndex > std::numeric_limits<uint32>::max() - (vertexCount - 1)) throw std::overflow_error("Draw vertex index exceeds the hardware field");
            const auto& layout = pipeline->GetShaders().GetVertexShader().GetCode().GetUserData();
            const auto* declaration = pipeline->GetVertexDeclaration();
            if (declaration)
            {
                for (const auto& element : declaration->GetElements())
                {
                    const auto index = layout.UsesVertexBuffersPerAttribute() ? element.AttributeIndex : element.StreamIndex;
                    if (index >= 32 || (layout.GetVertexBufferUsageMask() & (uint32{1} << index)) == 0) continue;
                    const auto snapshot = vertexStreams.Capture(element.StreamIndex);
                    const std::uint64_t lastIndex = element.bUseInstanceIndex ? instanceCount - 1 : static_cast<std::uint64_t>(baseVertexIndex) + vertexCount - 1;
                    const auto end = snapshot.Offset + lastIndex * element.Stride + element.Offset + VertexDeclaration::GetElementSize(element.Type);
                    if (end > snapshot.Storage->Size()) throw std::out_of_range("Draw vertex fetch exceeds its buffer");
                }
            }
            prepareDraw(commands, baseVertexIndex, 0, {});
            sceAgcDcbSetNumInstances(&commands, instanceCount);
            sceAgcDcbDrawIndexAuto(&commands, vertexCount, 0);
        });
    }

    void GraphicsContext::prepareDraw(CommandBuffer& commands, uint32 baseVertexIndex, uint32 firstInstance, std::shared_ptr<BufferAllocation> indexBuffer, std::shared_ptr<BufferAllocation> arguments)
    {
        const auto& layout = pipeline->GetShaders().GetVertexShader().GetCode().GetUserData();
        const auto* declaration = pipeline->GetVertexDeclaration();
        auto vertexBuffers = declaration ? vertexStreams.CaptureTable(*declaration, layout) : std::make_unique<VertexBufferTable>(0, std::span<const VertexBufferBinding>{}, layout.GetAddressHigh());
        auto vertexBindings = vertexParameters->CreateBindings();
        auto pixelBindings = pixelParameters ? pixelParameters->CreateBindings() : nullptr;
        draws.push_back({pipeline, std::move(vertexBindings), std::move(pixelBindings), std::move(vertexBuffers), std::move(indexBuffer), std::move(arguments)});
        const auto& draw = draws.back();
        Shader::UserDataInputs inputs;
        inputs.BaseVertex = baseVertexIndex;
        inputs.StartInstance = firstInstance;
        if (draw.indexBuffer)
        {
            const std::array ranges{GpuMemoryRange{draw.indexBuffer->Data(), draw.indexBuffer->Size()}};
            WriteMemoryAcquire(commands, ranges);
        }
        draw.pipeline->GetShaders().Record(commands, *draw.vertexBindings, draw.pixelBindings.get(), *draw.vertexBuffers, inputs);
        draw.pipeline->RecordColorState(commands, stencilReference, blendFactor);
    }

    void GraphicsContext::RHIDrawIndexedPrimitive(FRHIBuffer* indexBuffer, int32 baseVertexIndex, uint32 firstInstance, uint32, uint32 startIndex, uint32 primitiveCount, uint32 instanceCount)
    {
        RecordCommands([&](CommandBuffer& commands)
        {
            if (!pipeline || !activeRenderPass) throw std::runtime_error("Draw requires a graphics pipeline and render pass");
            activeRenderPass->ValidatePipeline(pipeline->GetDescription());
            if (!indexBuffer || !EnumHasAnyFlags(indexBuffer->GetUsage(), EBufferUsageFlags::IndexBuffer)) throw std::invalid_argument("Indexed draw requires an index buffer");
            const auto stride = indexBuffer->GetStride();
            if (stride != 2 && stride != 4) throw std::invalid_argument("Index stride must be two or four bytes");
            const auto indexCount = pipeline->GetVertexCount(primitiveCount);
            if (indexCount == 0) return;
            instanceCount = std::max(1u, instanceCount);
            if (firstInstance > std::numeric_limits<uint32>::max() - (instanceCount - 1)) throw std::overflow_error("Draw instance index exceeds the hardware field");
            auto indices = static_cast<Buffer*>(indexBuffer)->Capture();
            const auto range = indices->GetRange(static_cast<std::size_t>(startIndex) * stride, static_cast<std::size_t>(indexCount) * stride);
            prepareDraw(commands, static_cast<uint32>(baseVertexIndex), firstInstance, std::move(indices));
            sceAgcDcbSetIndexSize(&commands, stride == 2 ? 0 : 1, 0);
            sceAgcDcbSetNumInstances(&commands, instanceCount);
            sceAgcDcbDrawIndex(&commands, indexCount, range.data(), 0);
        });
    }

    void GraphicsContext::recordIndirectDraw(CommandBuffer& commands, FRHIBuffer* argumentBuffer, uint32 argumentOffset, std::shared_ptr<BufferAllocation> indices, uint32 indexStride)
    {
        if (!pipeline || !activeRenderPass) throw std::runtime_error("Indirect draw requires a graphics pipeline and render pass");
        activeRenderPass->ValidatePipeline(pipeline->GetDescription());
        if (!argumentBuffer || !EnumHasAnyFlags(argumentBuffer->GetUsage(), EBufferUsageFlags::DrawIndirect)) throw std::invalid_argument("Indirect draw requires an indirect argument buffer");
        if (argumentOffset % 4 != 0) throw std::invalid_argument("Indirect draw argument offset must be aligned to four bytes");
        auto arguments = static_cast<Buffer*>(argumentBuffer)->Capture();
        arguments->GetRange(argumentOffset, indices ? 20 : 16);
        prepareDraw(commands, 0, 0, indices, arguments);
        WriteIndirectDraw(commands, pipeline->GetShaders().GetVertexShader().GetCode().GetUserData(), *arguments, argumentOffset, indices.get(), indexStride);
    }

    void GraphicsContext::RHIDrawPrimitiveIndirect(FRHIBuffer* argumentBuffer, uint32 argumentOffset)
    {
        RecordCommands([&](CommandBuffer& commands) { recordIndirectDraw(commands, argumentBuffer, argumentOffset, {}, 0); });
    }

    void GraphicsContext::RHIDrawIndexedPrimitiveIndirect(FRHIBuffer* indexBuffer, FRHIBuffer* argumentBuffer, uint32 argumentOffset)
    {
        RecordCommands([&](CommandBuffer& commands)
        {
            if (!indexBuffer || !EnumHasAnyFlags(indexBuffer->GetUsage(), EBufferUsageFlags::IndexBuffer)) throw std::invalid_argument("Indexed indirect draw requires an index buffer");
            const auto stride = indexBuffer->GetStride();
            if (stride != 2 && stride != 4) throw std::invalid_argument("Index stride must be two or four bytes");
            recordIndirectDraw(commands, argumentBuffer, argumentOffset, static_cast<Buffer*>(indexBuffer)->Capture(), stride);
        });
    }

    void GraphicsContext::ValidateFinalization() const
    {
        if (activeRenderPass) throw std::runtime_error("Cannot finalize commands inside a render pass");
    }

    void GraphicsContext::RHIBeginRenderPass(const FRHIRenderPassInfo& info, const TCHAR*)
    {
        RecordCommands([&](CommandBuffer& commands)
        {
            if (activeRenderPass) throw std::runtime_error("A render pass is already active");
            auto pass = std::make_unique<RenderPass>(info, GetAddressHigh());
            renderPasses.push_back(std::move(pass));
            activeRenderPass = renderPasses.back().get();
            activeRenderPass->Begin(commands);
        });
    }

    void GraphicsContext::RHIEndRenderPass()
    {
        RecordCommands([&](CommandBuffer& commands)
        {
            if (!activeRenderPass) throw std::runtime_error("No render pass is active");
            activeRenderPass->End(commands);
            activeRenderPass = nullptr;
        });
    }

    void GraphicsContext::RHISetGraphicsPipelineState(FRHIGraphicsPipelineState* pipelineState, uint32 stencilRef, bool applyAdditionalState)
    {
        RecordCommands([&](CommandBuffer&)
        {
            if (!pipelineState) throw std::invalid_argument("Graphics pipeline is null");
            if (stencilRef > std::numeric_limits<uint8>::max()) throw std::out_of_range("Stencil reference exceeds eight bits");
            auto* nextPipeline = static_cast<GraphicsPipeline*>(pipelineState);
            auto& vertex = nextPipeline->GetShaders().GetVertexShader();
            auto* pixel = nextPipeline->GetShaders().GetPixelShader();
            auto nextVertex = vertexParameters && &vertexParameters->GetShader() == &vertex ? std::make_unique<ShaderParameters>(*vertexParameters) : std::make_unique<ShaderParameters>(vertex);
            std::unique_ptr<ShaderParameters> nextPixel;
            if (pixel) nextPixel = pixelParameters && &pixelParameters->GetShader() == pixel ? std::make_unique<ShaderParameters>(*pixelParameters) : std::make_unique<ShaderParameters>(*pixel);
            if (applyAdditionalState)
            {
                ApplyStaticUniformBuffers(*nextVertex);
                if (nextPixel) ApplyStaticUniformBuffers(*nextPixel);
            }
            pipeline = nextPipeline;
            vertexParameters = std::move(nextVertex);
            pixelParameters = std::move(nextPixel);
            stencilReference = stencilRef;
        });
    }

    void GraphicsContext::RHISetShaderParameters(FRHIGraphicsShader* shader, TConstArrayView<uint8> parametersData, TConstArrayView<FRHIShaderParameter> shaderParameters, TConstArrayView<FRHIShaderParameterResource> resourceParameters, TConstArrayView<FRHIShaderParameterResource> bindlessParameters)
    {
        RecordCommands([&](CommandBuffer&)
        {
            if (!shader || !pipeline) throw std::invalid_argument("Graphics parameters require an active pipeline and shader");
            if (!bindlessParameters.IsEmpty()) throw std::runtime_error("Bindless graphics parameters are not implemented");
            ShaderParameters* parameters = nullptr;
            if (vertexParameters && &vertexParameters->GetShader() == shader) parameters = vertexParameters.get();
            else if (pixelParameters && &pixelParameters->GetShader() == shader) parameters = pixelParameters.get();
            if (!parameters) throw std::invalid_argument("Graphics parameters require a shader from the active pipeline");
            parameters->SetConstants(parametersData, shaderParameters);
            parameters->SetResources(resourceParameters);
        });
    }

    void GraphicsContext::RHISetStencilRef(uint32 reference)
    {
        RecordCommands([&](CommandBuffer&)
        {
            if (reference > std::numeric_limits<uint8>::max()) throw std::out_of_range("Stencil reference exceeds eight bits");
            stencilReference = reference;
        });
    }

    void GraphicsContext::RHISetStreamSource(uint32 streamIndex, FRHIBuffer* vertexBuffer, uint32 offset)
    {
        RecordCommands([&](CommandBuffer&) { vertexStreams.Set(streamIndex, vertexBuffer, offset); });
    }

    void GraphicsContext::RHISetViewport(float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
    {
        RecordCommands([&](CommandBuffer& commands)
        {
            WriteViewport(commands, 0, {minX, minY, minZ, maxX, maxY, maxZ});
            WriteViewportScissor(commands, 0, {0, 0, 32767, 32767});
        });
    }

    void GraphicsContext::RHISetMultipleViewports(uint32 count, const FViewportBounds* data)
    {
        RecordCommands([&](CommandBuffer& commands)
        {
            if (!data || count == 0 || count > 16) throw std::invalid_argument("Multiple viewports require one to sixteen bounds");
            for (uint32 index = 0; index < count; ++index)
            {
                const auto& viewport = data[index];
                if (viewport.Width < 0 || viewport.Height < 0) throw std::out_of_range("Viewport dimensions must be nonnegative");
                WriteViewport(commands, index, {viewport.TopLeftX, viewport.TopLeftY, viewport.MinDepth, viewport.TopLeftX + viewport.Width, viewport.TopLeftY + viewport.Height, viewport.MaxDepth});
                WriteViewportScissor(commands, index, {0, 0, 32767, 32767});
            }
        });
    }

    void GraphicsContext::RHISetScissorRect(bool enable, uint32 minX, uint32 minY, uint32 maxX, uint32 maxY)
    {
        RecordCommands([&](CommandBuffer& commands)
        {
            WriteScissor(commands, enable ? ScissorRect{minX, minY, maxX, maxY} : ScissorRect{0, 0, 32767, 32767});
        });
    }
}
