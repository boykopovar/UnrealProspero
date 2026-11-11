#ifndef UNREALPROSPERO_RHI_GEOMETRY_SHADER_H
#define UNREALPROSPERO_RHI_GEOMETRY_SHADER_H

#include "UnrealProspero/RHI/Shader/ShaderResource.h"

namespace UnrealProspero::RHI
{
    using GeometryShader = ShaderResource<FRHIGeometryShader, Shader::ShaderStage::Geometry>;
}

#endif
