#include "AnyUnreal/RHI/Shader/ShaderCode.h"
#include "AnyUnreal/RHI/Shader/AgcHeader.h"
#include "Serialization/MemoryReader.h"
#include "ShaderCore.h"
#include "Misc/SecureHash.h"
#include <bit>
#include <stdexcept>

namespace AnyUnreal::RHI
{
    namespace
    {
        std::vector<std::uint8_t> copyCode(TConstArrayView<uint8> source)
        {
            if (source.Num() < 4 || !source.GetData()) throw std::invalid_argument("Unreal shader code is empty or truncated");
            return {source.begin(), source.end()};
        }

        void readResourceTable(std::span<const std::uint8_t> bytes, FShaderResourceTable& table)
        {
            const FMemoryView memory(bytes.data(), bytes.size());
            FMemoryReaderView bounds(memory, true);
            uint32 bits = 0;
            bounds << bits;
            for (unsigned index = 0; index < 6; ++index)
            {
                int32 count = 0;
                bounds << count;
                if (bounds.IsError() || count < 0 || count > (bounds.TotalSize() - bounds.Tell()) / static_cast<int64>(sizeof(uint32))) throw std::invalid_argument("Invalid Unreal shader resource table length");
                bounds.Seek(bounds.Tell() + static_cast<int64>(count) * sizeof(uint32));
            }
            if (bounds.Tell() != bounds.TotalSize()) throw std::invalid_argument("Trailing Unreal shader resource table data");
            FMemoryReaderView reader(memory, true);
            reader << table;
            if (reader.IsError() || reader.Tell() != reader.TotalSize()) throw std::invalid_argument("Cannot deserialize Unreal shader resource table");
            if (table.ResourceTableLayoutHashes.Num() > 32 || static_cast<unsigned>(table.ResourceTableLayoutHashes.Num()) < std::bit_width(table.ResourceTableBits)) throw std::invalid_argument("Unreal shader resource table mask exceeds its layouts");
        }
    }

    ShaderCode::ShaderCode(TConstArrayView<uint8> source, Shader::ShaderStage stage, Shader::AgcTarget target) : storage(copyCode(source)), binary(Shader::DecodeUnrealShaderCode(storage)), userData(Shader::UserDataLayout::Read(binary.Metadata, stage))
    {
        if (binary.Stage != stage || binary.Target != target) throw std::invalid_argument("Unreal shader code does not match the requested stage or platform");
        const FShaderCodeReader reader(source);
        const auto binarySize = static_cast<std::size_t>(binary.ResourceTable.data() + binary.ResourceTable.size() - storage.data());
        if (reader.GetActualShaderCodeSize() != binarySize) throw std::invalid_argument("Unreal shader code size does not match its AGC envelope");
        FSHA1::HashBuffer(storage.data(), binarySize, identity.data());
        readResourceTable(binary.ResourceTable, resourceTable);
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
