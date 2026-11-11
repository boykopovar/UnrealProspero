#include "AnyUnreal/Shader/AgcCompiler.h"
#include "AnyUnreal/Shader/EntryPoint.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include <exception>
#include <limits>
#include <stdexcept>

namespace AnyUnreal::Shader
{
    namespace
    {
        const TCHAR* stageName(EShaderFrequency stage)
        {
            switch (stage)
            {
                case SF_Vertex: return TEXT("vertex");
                case SF_Pixel: return TEXT("fragment");
                case SF_Compute: return TEXT("compute");
                default: throw std::invalid_argument("ADK SPIR-V command does not support this shader stage");
            }
        }

        const TCHAR* targetName(AgcTarget target)
        {
            switch (target)
            {
                case AgcTarget::Windows: return TEXT("windows");
                case AgcTarget::Linux: return TEXT("linux");
                case AgcTarget::Ps5: return TEXT("ps5");
                default: throw std::invalid_argument("Invalid AGC shader target");
            }
        }

        std::vector<std::uint8_t> readOutput(const FString& directory, const TCHAR* name)
        {
            TArray<uint8> bytes;
            const FString path = FPaths::Combine(directory, name);
            if (!FFileHelper::LoadFileToArray(bytes, *path) || bytes.IsEmpty()) throw std::runtime_error("Missing or empty ADK shader output: " + std::string(TCHAR_TO_UTF8(*path)));
            return {bytes.GetData(), bytes.GetData() + bytes.Num()};
        }
    }

    AgcCompilation CompileAgc(std::span<const std::uint32_t> words, std::span<const DescriptorBinding> descriptors, EShaderFrequency stage, AgcTarget target, const FString& entryPoint, const FString& executable, const FString& workingRoot)
    {
        if (words.size() < 5 || words.front() != 0x07230203 || words.size() > static_cast<std::size_t>(std::numeric_limits<int32>::max() / 4)) throw std::invalid_argument("Invalid SPIR-V input for ADK");
        if (entryPoint.IsEmpty()) throw std::invalid_argument("Shader entry point is empty");
        for (const TCHAR character : entryPoint)
        {
            if (character == 0) throw std::invalid_argument("Shader entry point contains a null character");
        }
        const auto normalized = RenameEntryPoint(words, TCHAR_TO_UTF8(*entryPoint), "main");
        words = normalized;
        FString arguments = FString::Printf(TEXT("spirv --input shader.spv --header header.bin --code code.bin --metadata bindings.adk --stage %s --target %s --entry main --omit-implicit-primitive-id true"), stageName(stage), targetName(target));
        if (stage == SF_Vertex) arguments += TEXT(" --dynamic-vertex-input");
        for (const auto& descriptor : descriptors)
        {
            if (descriptor.Type.empty()) throw std::invalid_argument("Shader descriptor type is empty");
            for (const char character : descriptor.Type)
            {
                if ((character < 'a' || character > 'z') && character != '-') throw std::invalid_argument("Invalid shader descriptor type token");
            }
            arguments += FString::Printf(TEXT(" --descriptor %u,%u,%s,%u,%u,%u"), descriptor.Set, descriptor.Binding, UTF8_TO_TCHAR(descriptor.Type.c_str()), descriptor.Count, descriptor.Offset, descriptor.Stride);
        }
        IFileManager& files = IFileManager::Get();
        if (executable.IsEmpty() || FPaths::IsRelative(executable) || !files.FileExists(*executable)) throw std::invalid_argument("ADK executable must be an existing absolute path");
        if (workingRoot.IsEmpty() || FPaths::IsRelative(workingRoot) || !files.DirectoryExists(*workingRoot)) throw std::invalid_argument("Shader working root must be an existing absolute directory");
        const FString directory = FPaths::Combine(workingRoot, FGuid::NewGuid().ToString(EGuidFormats::Digits));
        if (files.DirectoryExists(*directory) || !files.MakeDirectory(*directory, false)) throw std::runtime_error("Cannot create shader working directory");
        AgcCompilation result;
        try
        {
            TArray<uint8> bytes;
            bytes.SetNumUninitialized(static_cast<int32>(words.size() * 4));
            for (std::size_t index = 0; index < words.size(); ++index)
            {
                for (unsigned byte = 0; byte < 4; ++byte) bytes[static_cast<int32>(index * 4 + byte)] = static_cast<uint8>(words[index] >> (byte * 8));
            }
            const FString inputPath = FPaths::Combine(directory, TEXT("shader.spv"));
            if (!FFileHelper::SaveArrayToFile(MakeArrayView(bytes), *inputPath)) throw std::runtime_error("Cannot write SPIR-V input for ADK");
            int32 exitCode = -1;
            FString standardOutput;
            FString standardError;
            const bool launched = FPlatformProcess::ExecProcess(*executable, *arguments, &exitCode, &standardOutput, &standardError, *directory, true);
            if (!launched || exitCode != 0) throw std::runtime_error("ADK shader compilation failed (" + std::to_string(exitCode) + "): " + TCHAR_TO_UTF8(*(standardOutput + standardError)));
            result.Header = readOutput(directory, TEXT("header.bin"));
            result.Code = readOutput(directory, TEXT("code.bin"));
            result.Metadata = readOutput(directory, TEXT("bindings.adk"));
        }
        catch (...)
        {
            if (!files.DeleteDirectory(*directory, true, true)) std::throw_with_nested(std::runtime_error("Cannot remove failed shader working directory"));
            throw;
        }
        if (!files.DeleteDirectory(*directory, true, true)) throw std::runtime_error("Cannot remove shader working directory");
        return result;
    }
}
