#ifndef UNREALPROSPERO_SHADER_GEOMETRY_LIBRARY_H
#define UNREALPROSPERO_SHADER_GEOMETRY_LIBRARY_H

#include "UnrealProspero/Shader/ShaderBinary.h"

namespace UnrealProspero::Shader
{
    struct GeometryVariantView
    {
        std::span<const std::uint8_t> Bytes;
        ShaderBinaryView Binary;
    };

    UNREALPROSPEROSHADERCOMMON_API std::vector<std::uint8_t> EncodeGeometryLibrary(std::span<const std::vector<std::uint8_t>> variants);
    UNREALPROSPEROSHADERCOMMON_API std::vector<GeometryVariantView> DecodeGeometryLibrary(std::span<const std::uint8_t> bytes);
}

#endif
