#include "UnrealProspero/Shader/Cook/Finalize.h"
#include "UnrealProspero/Shader/Cook/Archive.h"
#include "UnrealProspero/Shader/Cook/GeometryInterface.h"
#include "UnrealProspero/Shader/Cook/GeometryVariant.h"
#include "UnrealProspero/Shader/GeometryLibrary.h"
#include "UnrealProspero/Shader/ResourceTableData.h"
#include "UnrealProspero/Shader/ShaderOutput.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "ShaderCore.h"
#include <algorithm>
#include <map>
#include <memory>
#include <stdexcept>
#include <utility>

namespace UnrealProspero::Shader::Cook
{
    namespace
    {
        struct VertexStage
        {
            ShaderIdentity Identity;
            StageData Stage;
        };

        struct GeometryStage
        {
            Archive* Owner;
            int32 Index;
            std::vector<std::uint8_t> Source;
        };

        std::span<const std::uint8_t> findStageData(const UnrealShaderCodeView& shader)
        {
            for (const auto& data : shader.OptionalData)
            {
                if (data.Key == static_cast<std::uint8_t>(EShaderOptionalDataKey::ObjectFile)) return data.Bytes;
            }
            throw std::invalid_argument("Vertex shader has no captured cook stage data");
        }

        UnrealShaderCodeView stripStageData(UnrealShaderCodeView shader)
        {
            std::erase_if(shader.OptionalData, [](const auto& data) { return data.Key == static_cast<std::uint8_t>(EShaderOptionalDataKey::ObjectFile); });
            return shader;
        }

        VertexStage readVertex(const UnrealShaderCodeView& shader, AgcTarget target)
        {
            auto stage = DecodeStageData(findStageData(shader));
            const auto native = DecodeShaderBinary(shader.Code);
            if (stage.Stage != SF_Vertex || native.Stage != ShaderStage::Vertex || stage.Target != target || native.Target != target) throw std::invalid_argument("Captured vertex shader has an incompatible stage or target");
            const auto bindings = BuildRuntimeBindings(stage.Spirv);
            ValidateLogicalBindings(native.Bindings, bindings);
            for (std::size_t index = 0; index < bindings.size(); ++index)
            {
                if (native.Bindings[index].Binding != bindings[index].Binding || native.Bindings[index].Offset != bindings[index].Offset) throw std::invalid_argument("Native vertex bindings disagree with its captured cook data");
            }
            if (!std::ranges::equal(native.ResourceTable, EncodeResourceTable(stage.Spirv.ResourceTable))) throw std::invalid_argument("Native vertex resource table disagrees with its captured cook data");
            ShaderIdentity identity;
            FSHA1::HashBuffer(shader.Code.data(), shader.Code.size(), identity.data());
            return {identity, std::move(stage)};
        }

        std::vector<std::uint8_t> compileVariants(const StageData& geometry, std::span<const VertexStage> vertices, const FString& executable, const FString& workingRoot)
        {
            std::map<ShaderIdentity, std::vector<std::uint8_t>> variants;
            for (const auto& vertex : vertices)
            {
                if (!CanLinkGeometryStages(vertex.Stage.Spirv.Words, TCHAR_TO_UTF8(*vertex.Stage.EntryPoint), geometry.Spirv.Words, TCHAR_TO_UTF8(*geometry.EntryPoint))) continue;
                auto code = CompileGeometryVariant(vertex.Stage, geometry, vertex.Identity, executable, workingRoot);
                const auto found = variants.find(vertex.Identity);
                if (found != variants.end())
                {
                    if (found->second != code) throw std::invalid_argument("A native vertex identity requires conflicting geometry variants");
                }
                else variants.emplace(vertex.Identity, std::move(code));
            }
            if (variants.empty()) throw std::invalid_argument("Geometry shader has no compatible cooked vertex shader");
            std::vector<std::vector<std::uint8_t>> code;
            code.reserve(variants.size());
            for (auto& [identity, bytes] : variants) code.push_back(std::move(bytes));
            return EncodeGeometryLibrary(code);
        }
    }

    void FinalizeArchives(std::span<const ArchivePaths> paths, AgcTarget target, const FString& executable, const FString& workingRoot)
    {
        if (paths.empty() || target > AgcTarget::Ps5) throw std::invalid_argument("Shader finalization requires archives for a valid target");
        if (FPaths::IsRelative(executable) || !IFileManager::Get().FileExists(*executable) || FPaths::IsRelative(workingRoot) || !IFileManager::Get().DirectoryExists(*workingRoot)) throw std::invalid_argument("Shader finalization requires an existing compiler and working directory");
        std::vector<std::unique_ptr<Archive>> archives;
        std::vector<VertexStage> vertices;
        std::vector<GeometryStage> geometry;
        for (std::size_t pathIndex = 0; pathIndex < paths.size(); ++pathIndex)
        {
            const auto& path = paths[pathIndex];
            if (FPaths::IsRelative(path.Source) || FPaths::IsRelative(path.Output) || FPaths::IsSamePath(path.Source, path.Output)) throw std::invalid_argument("Shader finalization requires separate absolute source and output paths");
            for (std::size_t previous = 0; previous < pathIndex; ++previous)
            {
                if (FPaths::IsSamePath(path.Source, paths[previous].Source) || FPaths::IsSamePath(path.Output, paths[previous].Output) || FPaths::IsSamePath(path.Source, paths[previous].Output) || FPaths::IsSamePath(path.Output, paths[previous].Source)) throw std::invalid_argument("Shader finalization paths overlap");
            }
            auto archive = std::make_unique<Archive>(path.Source);
            const auto entries = archive->GetIndex().GetShaderEntries();
            for (int32 index = 0; index < entries.Num(); ++index)
            {
                const auto bytes = archive->GetCode(index);
                const auto shader = DecodeUnrealShaderEnvelope(std::span(bytes.GetData(), static_cast<std::size_t>(bytes.Num())));
                if (entries[index].GetFrequency() == SF_Vertex)
                {
                    auto vertex = readVertex(shader, target);
                    const auto found = std::find_if(vertices.begin(), vertices.end(), [&](const auto& existing) { return existing.Identity == vertex.Identity && existing.Stage.EntryPoint == vertex.Stage.EntryPoint && existing.Stage.Spirv.Words == vertex.Stage.Spirv.Words; });
                    if (found == vertices.end()) vertices.push_back(std::move(vertex));
                    archive->SetCode(index, EncodeUnrealShaderEnvelope(stripStageData(shader)));
                }
                else if (entries[index].GetFrequency() == SF_Geometry)
                {
                    geometry.push_back({archive.get(), index, {bytes.GetData(), bytes.GetData() + bytes.Num()}});
                }
                else
                {
                    const auto native = DecodeShaderBinary(shader.Code);
                    const auto expected = entries[index].GetFrequency() == SF_Pixel ? ShaderStage::Pixel : entries[index].GetFrequency() == SF_Compute ? ShaderStage::Compute : throw std::invalid_argument("Shader archive contains an unsupported stage");
                    if (native.Stage != expected || native.Target != target) throw std::invalid_argument("Shader archive entry has an incompatible stage or target");
                }
            }
            archives.push_back(std::move(archive));
        }
        std::map<std::vector<std::uint8_t>, std::vector<std::uint8_t>> libraries;
        for (const auto& entry : geometry)
        {
            auto shader = DecodeUnrealShaderEnvelope(entry.Source);
            const std::vector<std::uint8_t> key(shader.Code.begin(), shader.Code.end());
            auto found = libraries.find(key);
            if (found == libraries.end())
            {
                const auto stage = DecodeStageData(shader.Code);
                if (stage.Stage != SF_Geometry || stage.Target != target) throw std::invalid_argument("Captured geometry shader has an incompatible stage or target");
                found = libraries.emplace(key, compileVariants(stage, vertices, executable, workingRoot)).first;
            }
            shader.Code = found->second;
            entry.Owner->SetCode(entry.Index, EncodeUnrealShaderEnvelope(stripStageData(std::move(shader))));
        }
        for (std::size_t index = 0; index < archives.size(); ++index) archives[index]->Save(paths[index].Output);
    }
}
