#include "AnyUnreal/RHI/Shader/ComputeShader.h"
#include "RHICoreShader.h"

namespace AnyUnreal::RHI
{
    ComputeShader::ComputeShader(TConstArrayView<uint8> source, const FShaderHash& hash, Shader::AgcTarget target) : code(source, Shader::ShaderStage::Compute, target), shader(code.GetBinary(), Shader::ShaderStage::Compute, target)
    {
        SetHash(hash);
        ShaderResourceTable = code.GetResourceTable();
        UE::RHICore::InitStaticUniformBufferSlots(this);
        FShaderCodeReader reader(code.GetUnrealCode());
        UE::RHICore::SetupShaderCodeValidationData(this, reader);
        UE::RHICore::SetupShaderDiagnosticData(this, reader);
    }

    ComputeShader::~ComputeShader() noexcept = default;
    const AgcShader& ComputeShader::GetNativeShader() const { return shader; }
    const ShaderCode& ComputeShader::GetCode() const { return code; }
}
