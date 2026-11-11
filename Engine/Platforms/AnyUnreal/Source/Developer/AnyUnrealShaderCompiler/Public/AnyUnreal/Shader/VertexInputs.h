#ifndef ANYUNREAL_SHADER_VERTEX_INPUTS_H
#define ANYUNREAL_SHADER_VERTEX_INPUTS_H

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace AnyUnreal::Shader
{
    enum class VertexScalarType { Float, SignedInteger, UnsignedInteger };

    struct VertexInput
    {
        std::uint32_t Location;
        std::uint32_t Components;
        VertexScalarType Type;
    };

    ANYUNREALSHADERCOMPILER_API std::vector<VertexInput> ReflectVertexInputs(std::span<const std::uint32_t> words, const std::string& entryPoint);
}

#endif
