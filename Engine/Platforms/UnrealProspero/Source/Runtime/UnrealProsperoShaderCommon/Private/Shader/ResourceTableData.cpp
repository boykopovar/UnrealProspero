#include "UnrealProspero/Shader/ResourceTableData.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include <bit>
#include <limits>
#include <stdexcept>

namespace UnrealProspero::Shader
{
    std::vector<std::uint8_t> EncodeResourceTable(const FShaderResourceTable& table)
    {
        TArray<uint8> bytes;
        FMemoryWriter writer(bytes, true);
        auto copy = table;
        writer << copy;
        if (writer.IsError()) throw std::runtime_error("Cannot serialize Unreal shader resource table");
        return {bytes.GetData(), bytes.GetData() + bytes.Num()};
    }

    void DecodeResourceTable(std::span<const std::uint8_t> bytes, FShaderResourceTable& table)
    {
        if (bytes.empty() || bytes.size() > static_cast<std::uint64_t>(std::numeric_limits<int64>::max())) throw std::invalid_argument("Invalid Unreal shader resource table size");
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
