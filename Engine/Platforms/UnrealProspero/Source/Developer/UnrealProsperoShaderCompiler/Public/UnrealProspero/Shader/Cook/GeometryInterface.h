#ifndef UNREALPROSPERO_SHADER_COOK_GEOMETRY_INTERFACE_H
#define UNREALPROSPERO_SHADER_COOK_GEOMETRY_INTERFACE_H

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace UnrealProspero::Shader::Cook
{
    UNREALPROSPEROSHADERCOMPILER_API bool CanLinkGeometryStages(std::span<const std::uint32_t> vertexWords, const std::string& vertexEntry, std::span<const std::uint32_t> geometryWords, const std::string& geometryEntry);
    UNREALPROSPEROSHADERCOMPILER_API std::vector<std::uint32_t> LinkGeometryInputs(std::span<const std::uint32_t> vertexWords, const std::string& vertexEntry, std::span<const std::uint32_t> geometryWords, const std::string& geometryEntry);
}

#endif
