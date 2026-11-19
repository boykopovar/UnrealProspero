#include "UnrealProspero/Shader/ShaderOutput.h"
#include "Serialization/MemoryWriter.h"
#include "ShaderCompilerCore.h"
#include <limits>
#include <stdexcept>

namespace UnrealProspero::Shader
{
    std::vector<std::uint8_t> EncodeCompiledShader(const SpirvCompilation& spirv, const AgcCompilation& agc, ShaderStage stage, AgcTarget target, const ShaderIdentity& vertexIdentity)
    {
        TArray<uint8> resourceTable;
        FMemoryWriter writer(resourceTable, true);
        auto table = spirv.ResourceTable;
        writer << table;
        if (writer.IsError()) throw std::runtime_error("Cannot serialize Unreal shader resource table");
        ShaderBinaryView binary{stage, target, agc.Header, agc.Code, agc.Metadata, {resourceTable.GetData(), static_cast<std::size_t>(resourceTable.Num())}, {}};
        binary.VertexIdentity = vertexIdentity;
        binary.ColorExportFormat = agc.ColorExportFormat;
        binary.ColorExports.reserve(agc.ColorExports.size());
        for (const auto& variant : agc.ColorExports) binary.ColorExports.push_back({variant.Format, variant.Header, variant.Code, variant.Metadata});
        binary.Bindings.reserve(spirv.Parameters.size());
        for (const auto& parameter : spirv.Parameters)
        {
            const auto& descriptor = spirv.Descriptors.at(parameter.DescriptorIndex);
            binary.Bindings.push_back({descriptor.Kind, parameter.ParameterIndex, descriptor.Set, descriptor.Binding, descriptor.Count, descriptor.Offset, descriptor.Stride, descriptor.BlockSize, descriptor.Type});
        }
        auto bytes = EncodeShaderBinary(binary);
        if (bytes.size() > static_cast<std::size_t>(std::numeric_limits<int32>::max())) throw std::overflow_error("Shader binary exceeds Unreal array limits");
        return bytes;
    }
}
