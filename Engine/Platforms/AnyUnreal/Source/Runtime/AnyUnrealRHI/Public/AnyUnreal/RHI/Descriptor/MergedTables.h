#ifndef ANYUNREAL_RHI_DESCRIPTOR_MERGED_TABLES_H
#define ANYUNREAL_RHI_DESCRIPTOR_MERGED_TABLES_H

#include "AnyUnreal/RHI/Descriptor/Tables.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API MergedDescriptorTables final
    {
    public:
        MergedDescriptorTables(std::span<const DescriptorTables* const> sources, std::uint32_t addressHigh);
        void Apply(Shader::UserDataInputs& inputs) const;
        std::vector<GpuMemoryRange> GetMemoryRanges() const;

    private:
        DescriptorStorage storage;
    };
}

#endif
