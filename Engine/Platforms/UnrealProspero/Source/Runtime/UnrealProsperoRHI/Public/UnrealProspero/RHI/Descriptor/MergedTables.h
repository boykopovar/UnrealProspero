#ifndef UNREALPROSPERO_RHI_DESCRIPTOR_MERGED_TABLES_H
#define UNREALPROSPERO_RHI_DESCRIPTOR_MERGED_TABLES_H

#include "UnrealProspero/RHI/Descriptor/Tables.h"

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API MergedDescriptorTables final
    {
    public:
        MergedDescriptorTables(std::span<const DescriptorTables* const> sources, std::uint32_t addressHigh);
        MergedDescriptorTables(std::span<const std::vector<DescriptorRange>> sources, std::uint32_t addressHigh);
        void Apply(Shader::UserDataInputs& inputs) const;
        std::vector<GpuMemoryRange> GetMemoryRanges() const;

    private:
        DescriptorStorage storage;
    };
}

#endif
