#ifndef ANYUNREAL_RHI_COMPUTE_SHADER_H
#define ANYUNREAL_RHI_COMPUTE_SHADER_H

#include "AnyUnreal/RHI/Shader/ShaderResource.h"

namespace AnyUnreal::RHI
{
    using ComputeShader = ShaderResource<FRHIComputeShader, Shader::ShaderStage::Compute>;
}

#endif
