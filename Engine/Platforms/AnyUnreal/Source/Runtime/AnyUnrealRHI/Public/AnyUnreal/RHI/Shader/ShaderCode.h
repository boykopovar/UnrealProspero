#ifndef ANYUNREAL_RHI_SHADER_CODE_H
#define ANYUNREAL_RHI_SHADER_CODE_H

#include "AnyUnreal/Shader/ShaderBinary.h"
#include "RHIResources.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API ShaderCode final
    {
    public:
        ShaderCode(TConstArrayView<uint8> source, Shader::ShaderStage stage, Shader::AgcTarget target);
        ShaderCode(const ShaderCode&) = delete;
        ShaderCode& operator=(const ShaderCode&) = delete;
        const Shader::ShaderBinaryView& GetBinary() const;
        const FShaderResourceTable& GetResourceTable() const;
        TConstArrayView<uint8> GetUnrealCode() const;

    private:
        std::vector<std::uint8_t> storage;
        Shader::ShaderBinaryView binary;
        FShaderResourceTable resourceTable;
    };
}

#endif
