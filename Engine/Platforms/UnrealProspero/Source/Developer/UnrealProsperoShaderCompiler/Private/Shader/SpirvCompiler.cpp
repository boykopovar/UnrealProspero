#include "UnrealProspero/Shader/SpirvCompiler.h"
#include "UnrealProspero/Shader/HlslCompiler.h"
#include "ShaderCompilerCommon.h"
#include "ShaderParameterParser.h"
#include "ShaderPreprocessTypes.h"
#include <limits>
#include <stdexcept>

namespace UnrealProspero::Shader
{
    SpirvCompilation CompileSpirv(const FShaderCompilerInput& input, const FShaderPreprocessOutput& preprocessed, FShaderCompilerOutput& output)
    {
        output.bSucceeded = false;
        if (!preprocessed.GetSucceeded()) throw std::invalid_argument("Shader preprocessing failed");
        if (!output.ParameterMap.GetParameterMap().IsEmpty()) throw std::invalid_argument("Shader output already contains parameter bindings");
        if (input.IsBindlessEnabled()) throw std::invalid_argument("Bindless AGC descriptor heaps are not implemented");
        output.Target = input.Target;
        try
        {
            FString source(preprocessed.GetSourceViewWide());
            FShaderParameterParser::FPlatformConfiguration configuration;
            configuration.Flags = EShaderParameterParserConfigurationFlags::AlwaysParseParams;
            if (input.RootParametersStructure)
            {
                configuration.StableConstantBufferType = TEXTVIEW("cbuffer");
                EnumAddFlags(configuration.Flags, EShaderParameterParserConfigurationFlags::UseStableConstantBuffer);
            }
            FShaderParameterParser parser(configuration);
            if (!parser.ParseAndModify(input, output.Errors, source)) throw std::runtime_error("Unreal shader parameter parsing failed");
            if (parser.DidModifyShader()) output.ModifiedShaderSource = source;
            const FTCHARToUTF8 utf8(*source, source.Len());
            SpirvCompilation result;
            result.Words = CompileHlslToSpirv(std::string_view(utf8.Get(), static_cast<std::size_t>(utf8.Length())), input, output);
            if (input.Target.GetFrequency() == SF_Vertex) result.VertexInputs = ReflectVertexInputs(result.Words, TCHAR_TO_UTF8(*input.EntryPointName));
            result.Descriptors = ReflectDescriptorLayout(result.Words, TCHAR_TO_UTF8(*input.EntryPointName));
            output.bSucceeded = true;
            result.Parameters = RegisterParameters(result.Descriptors, input, parser, output);
            if (!output.bSucceeded) throw std::runtime_error("Unreal shader resource registration failed");
            parser.ValidateShaderParameterTypes(input, false, output);
            if (!output.bSucceeded) throw std::runtime_error("Unreal shader parameter validation failed");
            FShaderCompilerResourceTable resourceTable;
            TBitArray<> usedUniformBuffers(false, std::numeric_limits<decltype(resourceTable.ResourceTableBits)>::digits);
            output.NumTextureSamplers = 0;
            for (const auto& parameter : result.Parameters)
            {
                const auto& descriptor = result.Descriptors[parameter.DescriptorIndex];
                if (descriptor.Kind == ResourceKind::UniformBuffer)
                {
                    if (parameter.ParameterIndex >= usedUniformBuffers.Num()) throw std::overflow_error("Uniform buffer slots exceed Unreal resource table limits");
                    usedUniformBuffers[parameter.ParameterIndex] = true;
                }
                if (descriptor.Kind == ResourceKind::Sampler) output.NumTextureSamplers += descriptor.Count;
            }
            if (!BuildResourceTableMapping(input.Environment.ResourceTableMap, input.Environment.UniformBufferMap, usedUniformBuffers, output.ParameterMap, resourceTable)) throw std::runtime_error("Unreal shader resource table construction failed");
            UE::ShaderCompilerCommon::BuildShaderResourceTable(resourceTable, result.ResourceTable);
            CullGlobalUniformBuffers(input.Environment.UniformBufferMap, output.ParameterMap);
            return result;
        }
        catch (...)
        {
            output.bSucceeded = false;
            throw;
        }
    }
}
