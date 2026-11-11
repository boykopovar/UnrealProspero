#ifndef ANYUNREAL_RHI_DESCRIPTOR_VERTEX_BUFFER_TABLE_H
#define ANYUNREAL_RHI_DESCRIPTOR_VERTEX_BUFFER_TABLE_H

#include "AnyUnreal/RHI/Memory/BufferAllocation.h"
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
    };

    class ANYUNREALRHI_API VertexBufferTable final
    {
    public:
        VertexBufferTable(std::uint32_t usageMask, std::span<const VertexBufferBinding> bindings, std::uint32_t addressHigh);
        void Apply(Shader::UserDataInputs& inputs) const;
        std::vector<GpuMemoryRange> GetMemoryRanges() const;

    private:
        std::vector<std::shared_ptr<BufferAllocation>> sources;
        std::unique_ptr<BufferAllocation> table;
    };
}

#endif
