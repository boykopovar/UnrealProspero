#include "UnrealProspero/Shader/ShaderOutput.h"
#include "UnrealProspero/Shader/ResourceTableData.h"
#include "ShaderCompilerCore.h"
#include <limits>
#include <stdexcept>

namespace UnrealProspero::Shader
{
    std::vector<std::uint8_t> EncodeCompiledShader(const SpirvCompilation& spirv, const AgcCompilation& agc, ShaderStage stage, AgcTarget target, const ShaderIdentity& vertexIdentity)
    {
        const auto resourceTable = EncodeResourceTable(spirv.ResourceTable);
        ShaderBinaryView binary{stage, target, agc.Header, agc.Code, agc.Metadata, resourceTable, {}};
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
