#ifndef ANYUNREAL_RHI_SHADER_CODE_H
#define ANYUNREAL_RHI_SHADER_CODE_H

#include "AnyUnreal/Shader/ShaderBinary.h"
#include "AnyUnreal/Shader/UserData.h"
#include "AnyUnreal/RHI/Shader/ResourceTable.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API ShaderCode final
    {
    public:
        ShaderCode(TConstArrayView<uint8> source, Shader::ShaderStage stage, Shader::AgcTarget target);
        ShaderCode(const ShaderCode&) = delete;
        ShaderCode& operator=(const ShaderCode&) = delete;
        const Shader::ShaderBinaryView& GetBinary() const;
        void ValidateVertex(const ShaderCode& vertex) const;
        const FShaderResourceTable& GetResourceTable() const;
        const Shader::UserDataLayout& GetUserData() const;
        const std::vector<ResourceTableBinding>& GetResourceBindings() const;
        TConstArrayView<uint8> GetUnrealCode() const;

    private:
        std::vector<std::uint8_t> storage;
        Shader::ShaderBinaryView binary;
        Shader::ShaderIdentity identity{};
        Shader::UserDataLayout userData;
        FShaderResourceTable resourceTable;
        std::vector<ResourceTableBinding> resourceBindings;
    };
}

#endif
