#include "AnyUnreal/Shader/ShaderCompiler.h"
#include "AnyUnreal/Shader/SpirvCompiler.h"
#include "Serialization/MemoryWriter.h"
#include "ShaderCompilerCore.h"
#include <limits>
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
            TArray<uint8> resourceTable;
            FMemoryWriter writer(resourceTable, true);
            writer << spirv.ResourceTable;
            if (writer.IsError()) throw std::runtime_error("Cannot serialize Unreal shader resource table");
            ShaderBinaryView binary{stage, target, agc.Header, agc.Code, agc.Metadata, {resourceTable.GetData(), static_cast<std::size_t>(resourceTable.Num())}, {}};
            std::vector<std::uint8_t> intermediate;
            intermediate.reserve(spirv.Words.size() * sizeof(std::uint32_t));
            for (const auto word : spirv.Words)
            {
                for (unsigned shift = 0; shift < 32; shift += 8) intermediate.push_back(static_cast<std::uint8_t>(word >> shift));
            }
            binary.Spirv = intermediate;
            binary.EntryPoint = TCHAR_TO_UTF8(*input.EntryPointName);
            binary.Bindings.reserve(spirv.Parameters.size());
            for (const auto& parameter : spirv.Parameters)
            {
                const auto& descriptor = spirv.Descriptors.at(parameter.DescriptorIndex);
                binary.Bindings.push_back({descriptor.Kind, parameter.ParameterIndex, descriptor.Set, descriptor.Binding, descriptor.Count, descriptor.Offset, descriptor.Stride, descriptor.BlockSize, descriptor.Type});
            }
            const auto bytes = EncodeShaderBinary(binary);
            if (bytes.size() > static_cast<std::size_t>(std::numeric_limits<int32>::max())) throw std::overflow_error("Shader binary exceeds Unreal array limits");
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
