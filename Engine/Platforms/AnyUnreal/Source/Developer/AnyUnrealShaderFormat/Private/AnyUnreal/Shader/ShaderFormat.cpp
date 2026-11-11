#include "AnyUnreal/Shader/ShaderFormat.h"
#include "AnyUnreal/Shader/ShaderCompiler.h"
#include "AnyUnreal/Shader/CompilerVersion.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Serialization/Archive.h"
#include "ShaderCompilerCore.h"
#include <array>
#include <memory>
#include <stdexcept>

namespace AnyUnreal::Shader
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
            throw std::invalid_argument("Unknown AnyUnreal shader format");
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
        version = HashCombine(GetTypeHash(compilerHash), HashCombine(GetShaderConductorVersion(), 3u));
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
        return TEXT("AnyUnreal");
    }

    void ShaderFormat::AppendToKeyString(FString& key) const
    {
        key += TEXT("_ANYUNREAL_AGC_");
        compilerHash.AppendString(key);
    }

    void ShaderFormat::CompilePreprocessedShader(const FShaderCompilerInput& input, const FShaderPreprocessOutput& preprocessed, FShaderCompilerOutput& output) const
    {
        output.bSucceeded = false;
        CompileShader(input, preprocessed, output, getTarget(input.ShaderFormat), executable, workingRoot);
    }
}
