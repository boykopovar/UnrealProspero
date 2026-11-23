#ifndef UNREALPROSPERO_SHADER_ENTRY_POINT_H
#define UNREALPROSPERO_SHADER_ENTRY_POINT_H

#include "CoreTypes.h"
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace UnrealProspero::Shader
{
    UNREALPROSPEROSHADERCOMPILER_API std::vector<std::uint32_t> RenameEntryPoint(std::span<const std::uint32_t> words, std::string_view original, std::string_view replacement);
}

#endif
