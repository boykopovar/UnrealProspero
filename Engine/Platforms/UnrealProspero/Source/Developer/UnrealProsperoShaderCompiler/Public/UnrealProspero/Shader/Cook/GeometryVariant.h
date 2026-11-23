#ifndef UNREALPROSPERO_SHADER_COOK_GEOMETRY_VARIANT_H
#define UNREALPROSPERO_SHADER_COOK_GEOMETRY_VARIANT_H

#include "UnrealProspero/Shader/Cook/StageData.h"

namespace UnrealProspero::Shader::Cook
{
    UNREALPROSPEROSHADERCOMPILER_API std::vector<std::uint8_t> CompileGeometryVariant(const StageData& vertex, const StageData& geometry, const ShaderIdentity& vertexIdentity, const FString& executable, const FString& workingRoot);
}

#endif
