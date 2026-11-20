#include "UnrealProspero/Shader/Cook/UnrealProsperoShaderCookerCommandlet.h"
#include "UnrealProspero/Shader/Cook/Finalize.h"
#include "HAL/FileManager.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include <stdexcept>

UUnrealProsperoShaderCookerCommandlet::UUnrealProsperoShaderCookerCommandlet()
{
    IsClient = false;
    IsServer = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UUnrealProsperoShaderCookerCommandlet::Main(const FString& parameters)
{
    try
    {
        FString source;
        FString output;
        FString compiler;
        FString workingRoot;
        if (!FParse::Value(*parameters, TEXT("Source="), source) || !FParse::Value(*parameters, TEXT("Output="), output) || !FParse::Value(*parameters, TEXT("Compiler="), compiler) || !FParse::Value(*parameters, TEXT("WorkingRoot="), workingRoot)) throw std::invalid_argument("Shader finalization requires Source, Output, Compiler and WorkingRoot");
        if (FPaths::IsRelative(source) || FPaths::IsRelative(output) || !IFileManager::Get().DirectoryExists(*source) || !IFileManager::Get().DirectoryExists(*output)) throw std::invalid_argument("Shader archive directories must exist and be absolute");
        TArray<FString> files;
        const auto pattern = FPaths::Combine(source, TEXT("ShaderArchive-*-SF_AGC_ANYPS5-SP_AGC_ANYPS5.ushaderbytecode"));
        IFileManager::Get().FindFiles(files, *pattern, true, false);
        files.Sort();
        if (files.IsEmpty()) throw std::invalid_argument("No PS5 shader source archives were cooked");
        std::vector<UnrealProspero::Shader::Cook::ArchivePaths> archives;
        archives.reserve(files.Num());
        for (const auto& file : files) archives.push_back({FPaths::Combine(source, file), FPaths::Combine(output, file)});
        UnrealProspero::Shader::Cook::FinalizeArchives(archives, UnrealProspero::Shader::AgcTarget::Ps5, compiler, workingRoot);
        UE_LOG(LogTemp, Display, TEXT("Finalized %d PS5 shader archives"), files.Num());
        return 0;
    }
    catch (const std::exception& error)
    {
        UE_LOG(LogTemp, Error, TEXT("%s"), UTF8_TO_TCHAR(error.what()));
        return 1;
    }
}
