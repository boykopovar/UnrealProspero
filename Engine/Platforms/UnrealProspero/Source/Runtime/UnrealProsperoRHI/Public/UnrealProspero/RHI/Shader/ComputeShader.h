#ifndef UNREALPROSPERO_RHI_COMPUTE_SHADER_H
#define UNREALPROSPERO_RHI_COMPUTE_SHADER_H

#include "UnrealProspero/RHI/Shader/ShaderResource.h"

namespace UnrealProspero::RHI
{
    using ComputeShader = ShaderResource<FRHIComputeShader, Shader::ShaderStage::Compute>;
}

#endif
