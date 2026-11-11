#ifndef UNREALPROSPERO_RHI_SHADER_AGC_SHADER_H
#define UNREALPROSPERO_RHI_SHADER_AGC_SHADER_H

#include "UnrealProspero/Kernel/DirectAllocation.h"
#include "UnrealProspero/Shader/ShaderBinary.h"
#include "SceShaders.hpp"

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API AgcShader final
    {
    public:
        AgcShader(const Shader::ShaderBinaryView& binary, Shader::ShaderStage expectedStage, Shader::AgcTarget expectedTarget);
        AgcShader(const AgcShader&) = delete;
        AgcShader& operator=(const AgcShader&) = delete;
        const ::Shader& Get() const;

    private:
        Kernel::DirectAllocation allocation;
        ::Shader* shader = nullptr;
    };
}

#endif
