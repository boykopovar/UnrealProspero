#include "AnyUnreal/RHI/Buffer/VertexStreams.h"
#include <limits>
#include <stdexcept>
#include <utility>

namespace AnyUnreal::RHI
{
    void VertexStreams::Set(uint32 index, FRHIBuffer* buffer, uint32 offset)
    {
        if (index > std::numeric_limits<uint8>::max()) throw std::out_of_range("Vertex stream index cannot be represented in a vertex declaration");
        if (!buffer)
        {
            bindings.erase(index);
            return;
        }
        if (!EnumHasAnyFlags(buffer->GetUsage(), EBufferUsageFlags::VertexBuffer)) throw std::invalid_argument("Stream source requires a vertex buffer");
        if (offset >= buffer->GetSize()) throw std::out_of_range("Vertex stream offset exceeds its buffer");
        bindings.insert_or_assign(index, Binding{static_cast<Buffer*>(buffer), offset});
    }

    VertexStreams::Snapshot VertexStreams::Capture(uint32 index) const
    {
        const auto found = bindings.find(index);
        if (found == bindings.end()) throw std::invalid_argument("Required vertex stream is not bound");
        const auto& binding = found->second;
        auto storage = binding.Resource->Capture();
        storage->GetRange(binding.Offset, 1);
        return {std::move(storage), binding.Offset};
    }
}
