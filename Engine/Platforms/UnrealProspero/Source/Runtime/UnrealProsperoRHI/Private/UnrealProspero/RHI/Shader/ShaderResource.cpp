#include "UnrealProspero/RHI/Shader/ShaderResource.h"
#include "RHICoreShader.h"

namespace UnrealProspero::RHI
{
    template class ShaderResource<FRHIComputeShader, Shader::ShaderStage::Compute>;
    template class ShaderResource<FRHIVertexShader, Shader::ShaderStage::Vertex>;
    template class ShaderResource<FRHIPixelShader, Shader::ShaderStage::Pixel>;

    void InitializeShaderMetadata(FRHIShader& resource, const ShaderCode& code, const FShaderHash& hash)
    {
        InitializeShaderMetadata(resource, code.GetUnrealCode(), hash);
    }

    void InitializeShaderMetadata(FRHIShader& resource, TConstArrayView<uint8> source, const FShaderHash& hash)
    {
        resource.SetHash(hash);
        UE::RHICore::InitStaticUniformBufferSlots(&resource);
        FShaderCodeReader reader(source);
        UE::RHICore::SetupShaderCodeValidationData(&resource, reader);
        UE::RHICore::SetupShaderDiagnosticData(&resource, reader);
    }
}
