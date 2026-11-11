#ifndef UNREALPROSPERO_RHI_DEVICE_SHADER_PLATFORM_H
#define UNREALPROSPERO_RHI_DEVICE_SHADER_PLATFORM_H

#include "UnrealProspero/Shader/ShaderBinary.h"
#include "RHIShaderPlatform.h"

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API EShaderPlatform ResolveShaderPlatform(Shader::AgcTarget target);
    UNREALPROSPERORHI_API void ConfigureShaderPlatform(EShaderPlatform platform);
}

#endif
