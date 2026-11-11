#ifndef ANYUNREAL_RHI_SHADER_BINDINGS_H
#define ANYUNREAL_RHI_SHADER_BINDINGS_H

#include "AnyUnreal/RHI/Buffer/UniformBufferBinding.h"
#include "AnyUnreal/RHI/Shader/ComputeShader.h"
#include "AnyUnreal/RHI/Shader/VertexShader.h"
#include "AnyUnreal/RHI/Shader/PixelShader.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API ShaderBindings final
    {
    public:
        explicit ShaderBindings(ComputeShader& shader);
        explicit ShaderBindings(VertexShader& shader);
        explicit ShaderBindings(PixelShader& shader);
        void Bind(TConstArrayView<FRHIShaderParameterResource> parameters);
        Shader::UserDataInputs Finalize(Shader::UserDataInputs inputs = {});

    private:
        ShaderBindings(FRHIShader& shader, const ShaderCode& code);
        void bindResource(FRHIShaderParameterResource::EType type, std::uint32_t index, FRHIResource* resource);
        TRefCountPtr<FRHIShader> shader;
        const ShaderCode& code;
        DescriptorTables descriptors;
        std::vector<TRefCountPtr<FRHIResource>> resources;
        std::vector<UniformBufferBinding> uniforms;
        bool finalized = false;
        bool failed = false;
    };
}

#endif
