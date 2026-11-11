#ifndef ANYUNREAL_SHADER_ENTRY_POINT_H
#define ANYUNREAL_SHADER_ENTRY_POINT_H

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace AnyUnreal::Shader
{
    ANYUNREALSHADERCOMPILER_API std::vector<std::uint32_t> RenameEntryPoint(std::span<const std::uint32_t> words, std::string_view original, std::string_view replacement);
}

#endif
