#ifndef ANYUNREAL_RHI_DEVICE_SHADER_PLATFORM_H
#define ANYUNREAL_RHI_DEVICE_SHADER_PLATFORM_H

#include "AnyUnreal/Shader/ShaderBinary.h"
#include "RHIShaderPlatform.h"

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API EShaderPlatform ResolveShaderPlatform(Shader::AgcTarget target);
    ANYUNREALRHI_API void ConfigureShaderPlatform(EShaderPlatform platform);
}

#endif
