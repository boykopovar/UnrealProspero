#include "UnrealProspero/Shader/ShaderFormat.h"
#include "UnrealProspero/Shader/ShaderCompiler.h"
#include "UnrealProspero/Shader/PipelineCompiler.h"
#include "UnrealProspero/Shader/CompilerVersion.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Serialization/Archive.h"
#include "ShaderCompilerCore.h"
#include <array>
#include <memory>
#include <stdexcept>

namespace UnrealProspero::Shader
{
    namespace
    {
        const FName windowsFormat(TEXT("SF_AGC_ANYWIN64"));
        const FName linuxFormat(TEXT("SF_AGC_ANYLINUX"));
        const FName ps5Format(TEXT("SF_AGC_ANYPS5"));

        AgcTarget getTarget(FName format)
        {
            if (format == windowsFormat) return AgcTarget::Windows;
            if (format == linuxFormat) return AgcTarget::Linux;
            if (format == ps5Format) return AgcTarget::Ps5;
            throw std::invalid_argument("Unknown UnrealProspero shader format");
        }

        FSHAHash hashExecutable(const FString& path)
        {
            std::unique_ptr<FArchive> reader(IFileManager::Get().CreateFileReader(*path));
            if (!reader || reader->TotalSize() <= 0) throw std::runtime_error("Cannot read ADK shader compiler");
            FSHA1 hash;
            std::array<uint8, 65536> buffer;
            int64 remaining = reader->TotalSize();
            while (remaining > 0)
            {
                const auto size = FMath::Min(remaining, static_cast<int64>(buffer.size()));
                reader->Serialize(buffer.data(), size);
                if (reader->IsError()) throw std::runtime_error("Cannot hash ADK shader compiler");
                hash.Update(buffer.data(), static_cast<uint64>(size));
                remaining -= size;
            }
            if (!reader->Close()) throw std::runtime_error("Cannot close ADK shader compiler");
            return hash.Finalize();
        }
    }

    ShaderFormat::ShaderFormat(const FString& executable, const FString& workingRoot) : executable(executable), workingRoot(workingRoot), version(0)
    {
        if (FPaths::IsRelative(executable) || FPaths::IsRelative(workingRoot)) throw std::invalid_argument("Shader compiler paths must be absolute");
        if (!IFileManager::Get().MakeDirectory(*workingRoot, true)) throw std::runtime_error("Cannot create shader compiler working directory");
        compilerHash = hashExecutable(executable);
        version = HashCombine(GetTypeHash(compilerHash), HashCombine(GetShaderConductorVersion(), 14u));
    }

    const IShaderPipelineCompiler* ShaderFormat::GetPipelineCompiler() const
    {
        return this;
    }

    void ShaderFormat::CompilePreprocessedShaderPipeline(TConstArrayView<FShaderPipelineStage> stages) const
    {
        for (const auto& stage : stages)
        {
            if (!stage.Input || !stage.Preprocessed || !stage.Output) throw std::invalid_argument("Null pipeline stage");
            stage.Output->bSucceeded = false;
        }
        try
        {
            if (stages.IsEmpty()) throw std::invalid_argument("Shader pipeline has no stages");
            std::vector<ShaderCompilationStage> inputs;
            inputs.reserve(static_cast<std::size_t>(stages.Num()));
            for (const auto& stage : stages) inputs.push_back({*stage.Input, *stage.Preprocessed, *stage.Output});
            CompileShaderPipeline(inputs, getTarget(stages[0].Input->ShaderFormat), executable, workingRoot);
        }
        catch (const std::exception& error)
        {
            for (const auto& stage : stages)
            {
                stage.Output->bSucceeded = false;
                stage.Output->AddErrorf(TEXT("%s"), UTF8_TO_TCHAR(error.what()));
            }
        }
    }

    uint32 ShaderFormat::GetVersion(FName format) const
    {
        return HashCombine(version, static_cast<uint32>(getTarget(format)));
    }

    void ShaderFormat::GetSupportedFormats(TArray<FName>& formats) const
    {
        formats.Add(windowsFormat);
        formats.Add(linuxFormat);
        formats.Add(ps5Format);
    }

    const TCHAR* ShaderFormat::GetPlatformIncludeDirectory() const
    {
        return TEXT("UnrealProspero");
    }

    void ShaderFormat::ModifyShaderCompilerInput(FShaderCompilerInput& input) const
    {
        input.Environment.SetDefine(TEXT("COMPILER_DXC"), true);
        input.Environment.SetDefine(TEXT("SM5_PROFILE"), true);
        input.Environment.SetDefine(TEXT("SUPPORTS_INDEPENDENT_SAMPLERS"), true);
    }

    void ShaderFormat::AppendToKeyString(FString& key) const
    {
        key += TEXT("_UNREALPROSPERO_AGC_");
        compilerHash.AppendString(key);
    }

    void ShaderFormat::CompilePreprocessedShader(const FShaderCompilerInput& input, const FShaderPreprocessOutput& preprocessed, FShaderCompilerOutput& output) const
    {
        output.bSucceeded = false;
        try
        {
            CompileShader(input, preprocessed, output, getTarget(input.ShaderFormat), executable, workingRoot);
        }
        catch (const std::exception& error)
        {
            output.bSucceeded = false;
            output.AddErrorf(TEXT("%s"), UTF8_TO_TCHAR(error.what()));
        }
    }
}
