#ifndef ANYUNREAL_RHI_GEOMETRY_SHADER_H
#define ANYUNREAL_RHI_GEOMETRY_SHADER_H

#include "AnyUnreal/RHI/Shader/ShaderResource.h"

namespace AnyUnreal::RHI
{
    using GeometryShader = ShaderResource<FRHIGeometryShader, Shader::ShaderStage::Geometry>;
}

#endif
