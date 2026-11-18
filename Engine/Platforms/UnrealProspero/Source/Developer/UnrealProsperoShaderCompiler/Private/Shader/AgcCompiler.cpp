#include "UnrealProspero/Shader/AgcCompiler.h"
#include "UnrealProspero/Shader/EntryPoint.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include <array>
#include <exception>
#include <limits>
#include <stdexcept>
#include <utility>

namespace UnrealProspero::Shader
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

        struct ShaderInput
        {
            const TCHAR* Name;
            std::span<const std::uint32_t> Words;
        };

        struct ShaderOutput
        {
            const TCHAR* Header;
            const TCHAR* Code;
            const TCHAR* Metadata;
        };

        std::vector<std::uint32_t> normalizeInput(std::span<const std::uint32_t> words, const FString& entryPoint)
        {
            if (words.size() < 5 || words.front() != 0x07230203 || words.size() > static_cast<std::size_t>(std::numeric_limits<int32>::max() / 4)) throw std::invalid_argument("Invalid SPIR-V input for ADK");
            if (entryPoint.IsEmpty()) throw std::invalid_argument("Shader entry point is empty");
            for (const TCHAR character : entryPoint)
            {
                if (character == 0) throw std::invalid_argument("Shader entry point contains a null character");
            }
            auto result = RenameEntryPoint(words, TCHAR_TO_UTF8(*entryPoint), "main");
            if (result.size() > static_cast<std::size_t>(std::numeric_limits<int32>::max() / 4)) throw std::invalid_argument("Normalized SPIR-V exceeds the file size limit");
            return result;
        }

        std::vector<AgcCompilation> runCompiler(FString arguments, std::span<const ShaderInput> inputs, std::span<const ShaderOutput> outputs, std::span<const DescriptorBinding> descriptors, const FString& executable, const FString& workingRoot)
        {
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
            std::vector<AgcCompilation> results;
            try
            {
                for (const auto& input : inputs)
                {
                    TArray<uint8> bytes;
                    bytes.SetNumUninitialized(static_cast<int32>(input.Words.size() * 4));
                    for (std::size_t index = 0; index < input.Words.size(); ++index)
                    {
                        for (unsigned byte = 0; byte < 4; ++byte) bytes[static_cast<int32>(index * 4 + byte)] = static_cast<uint8>(input.Words[index] >> (byte * 8));
                    }
                    const FString inputPath = FPaths::Combine(directory, input.Name);
                    if (!FFileHelper::SaveArrayToFile(MakeArrayView(bytes), *inputPath)) throw std::runtime_error("Cannot write SPIR-V input for ADK");
                }
                int32 exitCode = -1;
                FString standardOutput;
                FString standardError;
                const bool launched = FPlatformProcess::ExecProcess(*executable, *arguments, &exitCode, &standardOutput, &standardError, *directory, true);
                if (!launched || exitCode != 0) throw std::runtime_error("ADK shader compilation failed (" + std::to_string(exitCode) + "): " + TCHAR_TO_UTF8(*(standardOutput + standardError)));
                for (const auto& output : outputs)
                {
                    results.push_back({readOutput(directory, output.Header), readOutput(directory, output.Code), readOutput(directory, output.Metadata)});
                }
            }
            catch (...)
            {
                if (!files.DeleteDirectory(*directory, true, true)) std::throw_with_nested(std::runtime_error("Cannot remove failed shader working directory"));
                throw;
            }
            if (!files.DeleteDirectory(*directory, true, true)) throw std::runtime_error("Cannot remove shader working directory");
            return results;
        }
    }

    AgcCompilation CompileAgc(std::span<const std::uint32_t> words, std::span<const DescriptorBinding> descriptors, EShaderFrequency stage, AgcTarget target, const FString& entryPoint, const FString& executable, const FString& workingRoot)
    {
        const auto normalized = normalizeInput(words, entryPoint);
        FString arguments = FString::Printf(TEXT("spirv --input shader.spv --header header.bin --code code.bin --metadata bindings.adk --stage %s --target %s --entry main --omit-implicit-primitive-id true"), stageName(stage), targetName(target));
        if (stage == SF_Vertex) arguments += TEXT(" --dynamic-vertex-input");
        const std::array inputs{ShaderInput{TEXT("shader.spv"), normalized}};
        const std::array outputs{ShaderOutput{TEXT("header.bin"), TEXT("code.bin"), TEXT("bindings.adk")}};
        auto results = runCompiler(arguments, inputs, outputs, descriptors, executable, workingRoot);
        return std::move(results.front());
    }

    AgcCompilation CompileGeometryAgc(std::span<const std::uint32_t> vertexWords, std::span<const std::uint32_t> geometryWords, std::span<const DescriptorBinding> descriptors, AgcTarget target, const FString& vertexEntryPoint, const FString& geometryEntryPoint, const FString& executable, const FString& workingRoot)
    {
        const auto vertex = normalizeInput(vertexWords, vertexEntryPoint);
        const auto geometry = normalizeInput(geometryWords, geometryEntryPoint);
        const FString arguments = FString::Printf(TEXT("shader-pipeline --input-format spirv --vertex vertex.spv --geometry geometry.spv --output compiled --target %s --entry main --dynamic-vertex-input --omit-implicit-primitive-id true"), targetName(target));
        const std::array inputs{ShaderInput{TEXT("vertex.spv"), vertex}, ShaderInput{TEXT("geometry.spv"), geometry}};
        const std::array outputs{ShaderOutput{TEXT("compiled/geometry.header"), TEXT("compiled/geometry.code"), TEXT("compiled/geometry.adk")}};
        auto results = runCompiler(arguments, inputs, outputs, descriptors, executable, workingRoot);
        return std::move(results.front());
    }

    AgcTessellationCompilation CompileTessellationAgc(std::span<const std::uint32_t> vertexWords, std::span<const std::uint32_t> controlWords, std::span<const std::uint32_t> evaluationWords, std::span<const DescriptorBinding> descriptors, AgcTarget target, std::uint32_t patchControlPoints, const FString& vertexEntryPoint, const FString& controlEntryPoint, const FString& evaluationEntryPoint, const FString& executable, const FString& workingRoot)
    {
        if (patchControlPoints == 0 || patchControlPoints > 32) throw std::invalid_argument("Invalid tessellation patch control point count");
        const auto vertex = normalizeInput(vertexWords, vertexEntryPoint);
        const auto control = normalizeInput(controlWords, controlEntryPoint);
        const auto evaluation = normalizeInput(evaluationWords, evaluationEntryPoint);
        const FString arguments = FString::Printf(TEXT("shader-pipeline --input-format spirv --vertex vertex.spv --control control.spv --evaluation evaluation.spv --output compiled --target %s --entry main --patch-control-points %u --omit-implicit-primitive-id true"), targetName(target), patchControlPoints);
        const std::array inputs{ShaderInput{TEXT("vertex.spv"), vertex}, ShaderInput{TEXT("control.spv"), control}, ShaderInput{TEXT("evaluation.spv"), evaluation}};
        const std::array outputs{ShaderOutput{TEXT("compiled/control.header"), TEXT("compiled/control.code"), TEXT("compiled/control.adk")}, ShaderOutput{TEXT("compiled/evaluation.header"), TEXT("compiled/evaluation.code"), TEXT("compiled/evaluation.adk")}};
        auto results = runCompiler(arguments, inputs, outputs, descriptors, executable, workingRoot);
        return {std::move(results[0]), std::move(results[1])};
    }
}
