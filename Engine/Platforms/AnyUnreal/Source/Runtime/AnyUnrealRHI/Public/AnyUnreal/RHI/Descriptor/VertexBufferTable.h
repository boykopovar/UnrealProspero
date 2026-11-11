#ifndef ANYUNREAL_RHI_DESCRIPTOR_VERTEX_BUFFER_TABLE_H
#define ANYUNREAL_RHI_DESCRIPTOR_VERTEX_BUFFER_TABLE_H

#include "AnyUnreal/RHI/Memory/BufferAllocation.h"
#include "AnyUnreal/RHI/Descriptor/BufferDescriptor.h"
#include "AnyUnreal/RHI/Command/MemoryAcquire.h"
#include "AnyUnreal/Shader/UserData.h"
#include <memory>
#include <vector>

namespace AnyUnreal::RHI
{
    struct VertexBufferBinding
    {
        std::uint32_t Index;
        std::shared_ptr<BufferAllocation> Storage;
        std::size_t Offset;
        std::uint32_t Stride;
        VertexFormat Format = VertexFormat::Untyped;
        std::uint32_t InstanceDivisor = 1;
        bool InstanceRate = false;
    };

    class ANYUNREALRHI_API VertexBufferTable final
    {
    public:
        VertexBufferTable(std::uint32_t usageMask, std::span<const VertexBufferBinding> bindings, std::uint32_t addressHigh, bool dynamic = false);
        void Apply(Shader::UserDataInputs& inputs) const;
        std::uint32_t GetUsageMask() const;
        std::vector<GpuMemoryRange> GetMemoryRanges() const;

    private:
        std::uint32_t usageMask;
        std::vector<std::shared_ptr<BufferAllocation>> sources;
        std::unique_ptr<BufferAllocation> table;
    };
}

#endif
