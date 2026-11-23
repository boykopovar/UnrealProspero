#ifndef UNREALPROSPERO_SHADER_RESOURCE_TABLE_DATA_H
#define UNREALPROSPERO_SHADER_RESOURCE_TABLE_DATA_H

#include "RHIResources.h"
#include <cstdint>
#include <span>
#include <vector>

namespace UnrealProspero::Shader
{
    UNREALPROSPEROSHADERCOMMON_API std::vector<std::uint8_t> EncodeResourceTable(const FShaderResourceTable& table);
    UNREALPROSPEROSHADERCOMMON_API void DecodeResourceTable(std::span<const std::uint8_t> bytes, FShaderResourceTable& table);
}

#endif
