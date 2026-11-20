#ifndef UNREALPROSPERO_SHADER_COOK_STAGE_DATA_H
#define UNREALPROSPERO_SHADER_COOK_STAGE_DATA_H

#include "UnrealProspero/Shader/AgcCompiler.h"
#include "UnrealProspero/Shader/SpirvCompiler.h"

namespace UnrealProspero::Shader::Cook
{
    struct StageData
    {
        EShaderFrequency Stage;
        AgcTarget Target;
        FString EntryPoint;
        SpirvCompilation Spirv;
    };

    UNREALPROSPEROSHADERCOMPILER_API std::vector<std::uint8_t> EncodeStageData(const StageData& data);
    UNREALPROSPEROSHADERCOMPILER_API StageData DecodeStageData(std::span<const std::uint8_t> bytes);
}

#endif
