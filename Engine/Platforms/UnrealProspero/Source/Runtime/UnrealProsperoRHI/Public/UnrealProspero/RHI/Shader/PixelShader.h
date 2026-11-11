#ifndef UNREALPROSPERO_RHI_PIXEL_SHADER_H
#define UNREALPROSPERO_RHI_PIXEL_SHADER_H

#include "UnrealProspero/RHI/Shader/ShaderResource.h"

namespace UnrealProspero::RHI
{
    using PixelShader = ShaderResource<FRHIPixelShader, Shader::ShaderStage::Pixel>;
}

#endif
