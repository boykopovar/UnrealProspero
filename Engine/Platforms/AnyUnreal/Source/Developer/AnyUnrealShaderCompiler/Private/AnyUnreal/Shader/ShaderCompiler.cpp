#include "AnyUnreal/Shader/ShaderCompiler.h"
#include "AnyUnreal/Shader/SpirvCompiler.h"
#include "AnyUnreal/Shader/ShaderOutput.h"
#include "ShaderCompilerCore.h"
#include <stdexcept>

namespace AnyUnreal::Shader
{
    void CompileShader(const FShaderCompilerInput& input, const FShaderPreprocessOutput& preprocessed, FShaderCompilerOutput& output, AgcTarget target, const FString& executable, const FString& workingRoot)
    {
        output.bSucceeded = false;
        try
        {
            if (!output.ShaderCode.GetWriteAccess().IsEmpty()) throw std::invalid_argument("Shader output already contains code");
            ShaderStage stage;
            switch (input.Target.GetFrequency())
            {
                case SF_Vertex: stage = ShaderStage::Vertex; break;
                case SF_Pixel: stage = ShaderStage::Pixel; break;
                case SF_Compute: stage = ShaderStage::Compute; break;
                default: throw std::invalid_argument("AGC SPIR-V compiler does not support this shader stage");
            }
            auto spirv = CompileSpirv(input, preprocessed, output);
            output.bSucceeded = false;
            const auto agc = CompileAgc(spirv.Words, spirv.Descriptors, input.Target.GetFrequency(), target, input.EntryPointName, executable, workingRoot);
            const auto bytes = EncodeCompiledShader(spirv, agc, stage, target);
            output.ShaderCode.GetWriteAccess().Append(bytes.data(), static_cast<int32>(bytes.size()));
            output.SerializeShaderCodeValidation();
            output.bSucceeded = true;
        }
        catch (...)
        {
            output.bSucceeded = false;
            throw;
        }
    }
}
