#ifndef UNREALPROSPERO_RHI_VERTEX_SHADER_H
#define UNREALPROSPERO_RHI_VERTEX_SHADER_H

#include "UnrealProspero/RHI/Shader/ShaderResource.h"

namespace UnrealProspero::RHI
{
    using VertexShader = ShaderResource<FRHIVertexShader, Shader::ShaderStage::Vertex>;
}

#endif
