#ifndef ANYUNREAL_RHI_VERTEX_SHADER_H
#define ANYUNREAL_RHI_VERTEX_SHADER_H

#include "AnyUnreal/RHI/Shader/ShaderResource.h"

namespace AnyUnreal::RHI
{
    using VertexShader = ShaderResource<FRHIVertexShader, Shader::ShaderStage::Vertex>;
}

#endif
