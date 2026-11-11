#include "AnyUnreal/RHI/Pipeline/GraphicsShaders.h"
#include "AnyUnreal/RHI/Shader/ShaderCommands.h"
#include "prx/libSceAgc/DcbState/include/Registers.hpp"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    namespace
    {
        GraphicsShaderLink createLink(VertexShader& vertex, PixelShader* pixel, std::uint32_t primitiveType)
        {
            if (pixel && vertex.GetCode().GetBinary().Target != pixel->GetCode().GetBinary().Target) throw std::invalid_argument("Graphics shaders require matching target platforms");
            if (pixel && vertex.GetCode().GetUserData().GetAddressHigh() != pixel->GetCode().GetUserData().GetAddressHigh()) throw std::invalid_argument("Graphics shaders require matching address apertures");
            return GraphicsShaderLink(vertex.GetNativeShader(), pixel ? &pixel->GetNativeShader() : nullptr, primitiveType);
        }
    }

    GraphicsShaders::GraphicsShaders(VertexShader& vertex, PixelShader* pixel, std::uint32_t primitiveType) : vertex(&vertex), pixel(pixel), link(createLink(vertex, pixel, primitiveType)) {}

    VertexShader& GraphicsShaders::GetVertexShader() const { return *vertex; }
    PixelShader* GraphicsShaders::GetPixelShader() const { return pixel.GetReference(); }

    void GraphicsShaders::Record(CommandBuffer& commands, ShaderBindings& vertexBindings, ShaderBindings* pixelBindings, const VertexBufferTable& vertexBuffers) const
    {
        if (vertexBuffers.GetUsageMask() != vertex->GetCode().GetUserData().GetVertexBufferUsageMask()) throw std::invalid_argument("Vertex descriptors do not match the graphics shader");
        if (&vertexBindings.GetShader() != vertex.GetReference()) throw std::invalid_argument("Vertex bindings do not belong to this graphics pipeline");
        if (static_cast<bool>(pixel) != (pixelBindings != nullptr)) throw std::invalid_argument("Pixel bindings do not match the graphics pipeline stages");
        if (pixelBindings && &pixelBindings->GetShader() != pixel.GetReference()) throw std::invalid_argument("Pixel bindings do not belong to this graphics pipeline");
        Shader::UserDataInputs vertexInputs;
        vertexBuffers.Apply(vertexInputs);
        vertexInputs = vertexBindings.Finalize(vertexInputs);
        Shader::UserDataInputs pixelInputs;
        if (pixelBindings) pixelInputs = pixelBindings->Finalize();
        auto ranges = vertexBindings.GetMemoryRanges();
        const auto vertexRanges = vertexBuffers.GetMemoryRanges();
        ranges.insert(ranges.end(), vertexRanges.begin(), vertexRanges.end());
        if (pixelBindings)
        {
            const auto pixelRanges = pixelBindings->GetMemoryRanges();
            ranges.insert(ranges.end(), pixelRanges.begin(), pixelRanges.end());
        }
        const auto& vertexNative = vertex->GetNativeShader();
        const auto* pixelNative = pixel ? &pixel->GetNativeShader() : nullptr;
        ranges.push_back({vertexNative.Get().code, vertexNative.Get().shader_size});
        if (pixelNative) ranges.push_back({pixelNative->Get().code, pixelNative->Get().shader_size});
        WriteMemoryAcquire(commands, ranges);
        link.Record(commands);
        for (const auto* shader : {&vertexNative, pixelNative})
        {
            if (!shader) continue;
            const auto& native = shader->Get();
            for (std::uint32_t index = 0; index < native.num_cx_registers; ++index) sceAgcDcbSetCxRegisterDirect(&commands, native.cx_registers[index]);
        }
        WriteShaderRegisters(commands, vertexNative, vertex->GetCode().GetUserData(), vertexInputs);
        if (pixelNative) WriteShaderRegisters(commands, *pixelNative, pixel->GetCode().GetUserData(), pixelInputs);
    }
}
