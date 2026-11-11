#ifndef ANYUNREAL_RHI_PIXEL_SHADER_H
#define ANYUNREAL_RHI_PIXEL_SHADER_H

#include "AnyUnreal/RHI/Shader/ShaderResource.h"

namespace AnyUnreal::RHI
{
    using PixelShader = ShaderResource<FRHIPixelShader, Shader::ShaderStage::Pixel>;
}

#endif
