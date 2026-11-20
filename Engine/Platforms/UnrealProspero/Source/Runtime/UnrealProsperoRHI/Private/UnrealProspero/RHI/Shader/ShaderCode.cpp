#include "UnrealProspero/RHI/Shader/ShaderCode.h"
#include "UnrealProspero/RHI/Shader/AgcHeader.h"
#include "UnrealProspero/Shader/ResourceTableData.h"
#include "ShaderCore.h"
#include "Misc/SecureHash.h"
#include <stdexcept>

namespace UnrealProspero::RHI
{
    namespace
    {
        std::vector<std::uint8_t> copyCode(TConstArrayView<uint8> source)
        {
            if (source.Num() < 4 || !source.GetData()) throw std::invalid_argument("Unreal shader code is empty or truncated");
            return {source.begin(), source.end()};
        }
    }

    ShaderCode::ShaderCode(TConstArrayView<uint8> source, Shader::ShaderStage stage, Shader::AgcTarget target) : storage(copyCode(source)), binary(Shader::DecodeUnrealShaderCode(storage)), userData(Shader::UserDataLayout::Read(binary.Metadata, stage))
    {
        if (binary.Stage != stage || binary.Target != target) throw std::invalid_argument("Unreal shader code does not match the requested stage or platform");
        const FShaderCodeReader reader(source);
        const auto binarySize = static_cast<std::size_t>(binary.ResourceTable.data() + binary.ResourceTable.size() - storage.data());
        if (reader.GetActualShaderCodeSize() != binarySize) throw std::invalid_argument("Unreal shader code size does not match its AGC envelope");
        FSHA1::HashBuffer(storage.data(), binarySize, identity.data());
        Shader::DecodeResourceTable(binary.ResourceTable, resourceTable);
        resourceBindings = ReadResourceTableBindings(resourceTable);
        ValidateAgcHeader(binary.Header, binary.Code, binary.Stage);
    }

    void ShaderCode::ValidateVertex(const ShaderCode& vertex) const
    {
        Shader::ValidateGeometryVertex(binary, vertex.binary, vertex.identity);
    }

    const Shader::ShaderBinaryView& ShaderCode::GetBinary() const { return binary; }
    const FShaderResourceTable& ShaderCode::GetResourceTable() const { return resourceTable; }
    const std::vector<ResourceTableBinding>& ShaderCode::GetResourceBindings() const { return resourceBindings; }
    const Shader::UserDataLayout& ShaderCode::GetUserData() const { return userData; }
    TConstArrayView<uint8> ShaderCode::GetUnrealCode() const { return {storage.data(), static_cast<int32>(storage.size())}; }
}
