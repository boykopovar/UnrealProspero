#ifndef UNREALPROSPERO_SHADER_VERTEX_INPUT_LOCATIONS_H
#define UNREALPROSPERO_SHADER_VERTEX_INPUT_LOCATIONS_H

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace UnrealProspero::Shader
{
    UNREALPROSPEROSHADERCOMPILER_API std::vector<std::uint32_t> NormalizeVertexInputLocations(std::span<const std::uint32_t> words, const std::string& entryPoint);
}

#endif
