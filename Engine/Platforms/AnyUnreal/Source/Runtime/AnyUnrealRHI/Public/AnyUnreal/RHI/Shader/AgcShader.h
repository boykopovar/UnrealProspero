#ifndef ANYUNREAL_RHI_SHADER_AGC_SHADER_H
#define ANYUNREAL_RHI_SHADER_AGC_SHADER_H

#include "AnyUnreal/Kernel/DirectAllocation.h"
#include "AnyUnreal/Shader/ShaderBinary.h"
#include "SceShaders.hpp"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API AgcShader final
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
