#include "UnrealProspero/Shader/GeometryCompiler.h"
#include "UnrealProspero/Shader/PipelineDescriptors.h"
#include "UnrealProspero/Shader/ShaderOutput.h"
#include "UnrealProspero/Shader/Cook/StageData.h"
#include "UnrealProspero/Shader/GeometryLibrary.h"
#include "ShaderCompilerCore.h"
#include "Misc/SecureHash.h"
#include <array>
#include <limits>
#include <stdexcept>

namespace UnrealProspero::Shader
{
    void CompileGeometryShaders(const ShaderCompilationStage& vertex, const ShaderCompilationStage& geometry, AgcTarget target, const FString& executable, const FString& workingRoot)
    {
        vertex.Output.bSucceeded = false;
        geometry.Output.bSucceeded = false;
        try
        {
            if (&vertex.Output == &geometry.Output) throw std::invalid_argument("Pipeline stages share an output");
            if (vertex.Input.Target.GetFrequency() != SF_Vertex || geometry.Input.Target.GetFrequency() != SF_Geometry) throw std::invalid_argument("Geometry pipeline requires vertex and geometry stages");
            if (vertex.Input.ShaderFormat != geometry.Input.ShaderFormat || vertex.Input.Target.GetPlatform() != geometry.Input.Target.GetPlatform()) throw std::invalid_argument("Pipeline stages require the same shader platform");
            if (!vertex.Output.ShaderCode.GetWriteAccess().IsEmpty() || !geometry.Output.ShaderCode.GetWriteAccess().IsEmpty()) throw std::invalid_argument("Pipeline output already contains shader code");
            auto vertexSpirv = CompileSpirv(vertex.Input, vertex.Preprocessed, vertex.Output);
            vertex.Output.bSucceeded = false;
            auto geometrySpirv = CompileSpirv(geometry.Input, geometry.Preprocessed, geometry.Output);
            geometry.Output.bSucceeded = false;
            const auto vertexCookData = Cook::EncodeStageData(SF_Vertex, target, vertex.Input.EntryPointName, vertexSpirv);
            const auto geometryCookData = Cook::EncodeStageData(SF_Geometry, target, geometry.Input.EntryPointName, geometrySpirv);
            const std::array stages{PipelineStageDescriptors{vertexSpirv.Words, vertexSpirv.Descriptors, TCHAR_TO_UTF8(*vertex.Input.EntryPointName)}, PipelineStageDescriptors{geometrySpirv.Words, geometrySpirv.Descriptors, TCHAR_TO_UTF8(*geometry.Input.EntryPointName)}};
            const auto descriptors = MergePipelineDescriptors(stages);
            const auto vertexAgc = CompileAgc(vertexSpirv.Words, vertexSpirv.Descriptors, SF_Vertex, target, vertex.Input.EntryPointName, executable, workingRoot);
            const auto geometryAgc = CompileGeometryAgc(vertexSpirv.Words, geometrySpirv.Words, descriptors, target, vertex.Input.EntryPointName, geometry.Input.EntryPointName, executable, workingRoot);
            const auto vertexBytes = EncodeCompiledShader(vertexSpirv, vertexAgc, ShaderStage::Vertex, target);
            ShaderIdentity vertexIdentity;
            FSHA1::HashBuffer(vertexBytes.data(), vertexBytes.size(), vertexIdentity.data());
            const auto geometryBytes = EncodeCompiledShader(geometrySpirv, geometryAgc, ShaderStage::Geometry, target, vertexIdentity);
            const auto geometryLibrary = EncodeGeometryLibrary(std::array{geometryBytes});
            if (geometryLibrary.size() > static_cast<std::size_t>(std::numeric_limits<int32>::max())) throw std::overflow_error("Geometry library exceeds Unreal array limits");
            vertex.Output.ShaderCode.GetWriteAccess().Append(vertexBytes.data(), static_cast<int32>(vertexBytes.size()));
            geometry.Output.ShaderCode.GetWriteAccess().Append(geometryLibrary.data(), static_cast<int32>(geometryLibrary.size()));
            vertex.Output.ShaderCode.AddOptionalData(EShaderOptionalDataKey::ObjectFile, vertexCookData.data(), static_cast<uint32>(vertexCookData.size()));
            geometry.Output.ShaderCode.AddOptionalData(EShaderOptionalDataKey::ObjectFile, geometryCookData.data(), static_cast<uint32>(geometryCookData.size()));
            vertex.Output.SerializeShaderCodeValidation();
            geometry.Output.SerializeShaderCodeValidation();
            vertex.Output.bSucceeded = true;
            geometry.Output.bSucceeded = true;
        }
        catch (...)
        {
            vertex.Output.bSucceeded = false;
            geometry.Output.bSucceeded = false;
            throw;
        }
    }
}
