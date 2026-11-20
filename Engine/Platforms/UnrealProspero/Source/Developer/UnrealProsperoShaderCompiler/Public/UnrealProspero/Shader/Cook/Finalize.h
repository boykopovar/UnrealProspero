#ifndef UNREALPROSPERO_SHADER_COOK_FINALIZE_H
#define UNREALPROSPERO_SHADER_COOK_FINALIZE_H

#include "UnrealProspero/Shader/ShaderBinary.h"
#include "Containers/UnrealString.h"

namespace UnrealProspero::Shader::Cook
{
    struct ArchivePaths
    {
        FString Source;
        FString Output;
    };

    UNREALPROSPEROSHADERCOMPILER_API void FinalizeArchives(std::span<const ArchivePaths> paths, AgcTarget target, const FString& executable, const FString& workingRoot);
}

#endif
