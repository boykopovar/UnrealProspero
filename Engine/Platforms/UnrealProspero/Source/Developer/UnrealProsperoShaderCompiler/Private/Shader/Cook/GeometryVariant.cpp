#include "UnrealProspero/Shader/Cook/GeometryVariant.h"
#include "UnrealProspero/Shader/PipelineDescriptors.h"
#include "UnrealProspero/Shader/ShaderOutput.h"
#include <algorithm>
#include <array>
#include <stdexcept>

namespace UnrealProspero::Shader::Cook
{
    std::vector<std::uint8_t> CompileGeometryVariant(const StageData& vertex, const StageData& geometry, const ShaderIdentity& vertexIdentity, const FString& executable, const FString& workingRoot)
    {
        if (vertex.Stage != SF_Vertex || geometry.Stage != SF_Geometry || vertex.Target != geometry.Target) throw std::invalid_argument("Offline geometry compilation requires vertex and geometry stages for the same target");
        if (std::ranges::none_of(vertexIdentity, [](auto value) { return value != 0; })) throw std::invalid_argument("Offline geometry compilation requires the native vertex identity");
        auto vertexWords = vertex.Spirv.Words;
        auto vertexDescriptors = vertex.Spirv.Descriptors;
        auto geometrySpirv = geometry.Spirv;
        const std::array stages{PipelineStageDescriptors{vertexWords, vertexDescriptors, TCHAR_TO_UTF8(*vertex.EntryPoint)}, PipelineStageDescriptors{geometrySpirv.Words, geometrySpirv.Descriptors, TCHAR_TO_UTF8(*geometry.EntryPoint)}};
        const auto descriptors = MergePipelineDescriptors(stages, PipelineDescriptorLayout::PreserveFirstStage);
        if (vertexWords != vertex.Spirv.Words || vertexDescriptors != vertex.Spirv.Descriptors) throw std::runtime_error("Offline geometry compilation changed the native vertex interface");
        const auto agc = CompileGeometryAgc(vertexWords, geometrySpirv.Words, descriptors, vertex.Target, vertex.EntryPoint, geometry.EntryPoint, executable, workingRoot);
        return EncodeCompiledShader(geometrySpirv, agc, ShaderStage::Geometry, geometry.Target, vertexIdentity);
    }
}
