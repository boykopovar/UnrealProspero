#ifndef ANYUNREAL_RHI_DESCRIPTOR_TABLES_H
#define ANYUNREAL_RHI_DESCRIPTOR_TABLES_H

#include "AnyUnreal/RHI/Memory/BufferAllocation.h"
#include "AnyUnreal/Shader/ShaderBinary.h"
#include "AnyUnreal/Shader/UserData.h"
#include "AnyUnreal/RHI/Command/MemoryAcquire.h"
#include <array>
#include <memory>

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API DescriptorTables final
    {
    public:
        DescriptorTables(std::span<const Shader::RuntimeBinding> bindings, std::uint32_t addressHigh);
        void SetDescriptor(Shader::ResourceKind kind, std::uint32_t parameterIndex, std::span<const std::byte> descriptor);
        void SetBuffer(Shader::ResourceKind kind, std::uint32_t parameterIndex, const void* address, std::size_t size);
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
        std::array<std::unique_ptr<BufferAllocation>, 4> tables;
    };
}

#endif
