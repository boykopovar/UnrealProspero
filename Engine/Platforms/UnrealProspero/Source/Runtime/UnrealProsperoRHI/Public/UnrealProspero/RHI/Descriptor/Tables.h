#ifndef UNREALPROSPERO_RHI_DESCRIPTOR_TABLES_H
#define UNREALPROSPERO_RHI_DESCRIPTOR_TABLES_H

#include "UnrealProspero/RHI/Descriptor/Storage.h"
#include "UnrealProspero/RHI/Descriptor/Range.h"

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API DescriptorTables final
    {
    public:
        DescriptorTables(std::span<const Shader::RuntimeBinding> bindings, std::uint32_t addressHigh);
        void SetDescriptor(Shader::ResourceKind kind, std::uint32_t parameterIndex, std::span<const std::byte> descriptor);
        void SetBuffer(Shader::ResourceKind kind, std::uint32_t parameterIndex, const void* address, std::size_t size);
        std::vector<DescriptorRange> GetRanges() const;
        void Apply(Shader::UserDataInputs& inputs) const;
        std::vector<GpuMemoryRange> GetMemoryRanges() const;

    private:
        struct Entry
        {
            Shader::RuntimeBinding binding;
            std::vector<bool> initialized;
        };

        Entry& findEntry(Shader::ResourceKind kind, std::uint32_t parameterIndex);
        std::vector<Entry> entries;
        void validateInitialized() const;
        DescriptorStorage storage;
    };
}

#endif
