#include "AnyUnreal/RHI/Shader/ShaderResource.h"
#include "RHICoreShader.h"

namespace AnyUnreal::RHI
{
    template class ShaderResource<FRHIComputeShader, Shader::ShaderStage::Compute>;
    template class ShaderResource<FRHIVertexShader, Shader::ShaderStage::Vertex>;
    template class ShaderResource<FRHIPixelShader, Shader::ShaderStage::Pixel>;
    template class ShaderResource<FRHIGeometryShader, Shader::ShaderStage::Geometry>;

    void InitializeShaderMetadata(FRHIShader& resource, const ShaderCode& code, const FShaderHash& hash)
    {
        resource.SetHash(hash);
        UE::RHICore::InitStaticUniformBufferSlots(&resource);
        FShaderCodeReader reader(code.GetUnrealCode());
        UE::RHICore::SetupShaderCodeValidationData(&resource, reader);
        UE::RHICore::SetupShaderDiagnosticData(&resource, reader);
    }
}
