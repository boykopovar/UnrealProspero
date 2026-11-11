#ifndef ANYUNREAL_RHI_SHADER_BINDINGS_H
#define ANYUNREAL_RHI_SHADER_BINDINGS_H

#include "AnyUnreal/RHI/Buffer/UniformBufferBinding.h"
#include "AnyUnreal/RHI/Shader/ComputeShader.h"
#include "AnyUnreal/RHI/Shader/VertexShader.h"
#include "AnyUnreal/RHI/Shader/PixelShader.h"
#include "AnyUnreal/Shader/ConstantData.h"
#include <map>
#include <set>

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API void ValidateShaderResourceParameter(FRHIShaderParameterResource::EType type, FRHIResource* resource);

    class ANYUNREALRHI_API ShaderBindings final
    {
    public:
        explicit ShaderBindings(ComputeShader& shader);
        explicit ShaderBindings(VertexShader& shader);
        explicit ShaderBindings(PixelShader& shader);
        const FRHIShader& GetShader() const;
        std::vector<GpuMemoryRange> GetMemoryRanges() const;
        void Bind(TConstArrayView<FRHIShaderParameterResource> parameters);
        void BindConstantData(const Shader::ConstantData& data);
        void BindConstants(TConstArrayView<uint8> data, TConstArrayView<FRHIShaderParameter> parameters);
        Shader::UserDataInputs Finalize(Shader::UserDataInputs inputs = {});

    private:
        ShaderBindings(FRHIShader& shader, const ShaderCode& code);
        void bindResource(FRHIShaderParameterResource::EType type, std::uint32_t index, FRHIResource* resource);
        TRefCountPtr<FRHIShader> shader;
        const ShaderCode& code;
        DescriptorTables descriptors;
        Shader::ConstantData constants;
        std::vector<TRefCountPtr<FRHIResource>> resources;
        std::vector<UniformBufferBinding> uniforms;
        std::vector<std::shared_ptr<BufferAllocation>> bufferAllocations;
        std::map<std::uint32_t, std::unique_ptr<BufferAllocation>> constantBuffers;
        std::set<std::uint32_t> uniformIndices;
        bool finalized = false;
        bool failed = false;
    };
}

#endif
