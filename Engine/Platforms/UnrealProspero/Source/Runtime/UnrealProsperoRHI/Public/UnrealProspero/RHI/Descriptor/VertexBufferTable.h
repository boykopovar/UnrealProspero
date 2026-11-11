#ifndef UNREALPROSPERO_RHI_DESCRIPTOR_VERTEX_BUFFER_TABLE_H
#define UNREALPROSPERO_RHI_DESCRIPTOR_VERTEX_BUFFER_TABLE_H

#include "UnrealProspero/RHI/Memory/BufferAllocation.h"
#include "UnrealProspero/RHI/Descriptor/BufferDescriptor.h"
#include "UnrealProspero/RHI/Command/MemoryAcquire.h"
#include "UnrealProspero/Shader/UserData.h"
#include <memory>
#include <vector>

namespace UnrealProspero::RHI
{
    struct VertexBufferBinding
    {
        std::uint32_t Index;
        std::shared_ptr<BufferAllocation> Storage;
        std::size_t Offset;
        std::uint32_t Stride;
        BufferFormat Format = BufferFormat::Untyped;
        std::uint32_t InstanceDivisor = 1;
        bool InstanceRate = false;
    };

    class UNREALPROSPERORHI_API VertexBufferTable final
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
