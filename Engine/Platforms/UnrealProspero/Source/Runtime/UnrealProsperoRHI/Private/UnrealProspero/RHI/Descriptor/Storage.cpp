#include "UnrealProspero/RHI/Descriptor/Storage.h"
#include <stdexcept>

namespace UnrealProspero::RHI
{
    DescriptorStorage::DescriptorStorage(const std::array<std::size_t, 4>& sizes, std::uint32_t addressHigh)
    {
        for (std::size_t index = 0; index < tables.size(); ++index)
        {
            if (sizes[index] != 0) tables[index] = std::make_unique<BufferAllocation>(sizes[index], addressHigh);
        }
    }

    void DescriptorStorage::Upload(std::uint32_t set, std::size_t offset, std::span<const std::byte> source)
    {
        if (set >= tables.size() || !tables[set]) throw std::out_of_range("Descriptor set has no storage");
        tables[set]->Upload(offset, source);
    }

    std::span<const std::byte> DescriptorStorage::GetRange(std::uint32_t set, std::size_t offset, std::size_t size) const
    {
        if (set >= tables.size() || !tables[set]) throw std::out_of_range("Descriptor set has no storage");
        return tables[set]->GetRange(offset, size);
    }

    void DescriptorStorage::Apply(Shader::UserDataInputs& inputs) const
    {
        for (std::size_t index = 0; index < tables.size(); ++index) inputs.DescriptorSets[index] = tables[index] ? reinterpret_cast<std::uintptr_t>(tables[index]->Data()) : 0;
    }

    std::vector<GpuMemoryRange> DescriptorStorage::GetMemoryRanges() const
    {
        std::vector<GpuMemoryRange> result;
        for (const auto& table : tables)
        {
            if (table) result.push_back({table->Data(), table->Size()});
        }
        return result;
    }
}
