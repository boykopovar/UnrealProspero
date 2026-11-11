#include "AnyUnreal/RHI/Buffer/VertexStreams.h"
#include "AnyUnreal/RHI/State/VertexDeclaration.h"
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

    std::unique_ptr<VertexBufferTable> VertexStreams::CaptureTable(const VertexDeclaration& declaration, const Shader::UserDataLayout& layout) const
    {
        const auto mask = layout.GetVertexBufferUsageMask();
        std::vector<VertexBufferBinding> captured;
        for (uint32 index = 0; index < 32; ++index)
        {
            if ((mask & (uint32{1} << index)) == 0) continue;
            const FVertexElement* selected = nullptr;
            for (const auto& element : declaration.GetElements())
            {
                const auto descriptorIndex = layout.UsesVertexBuffersPerAttribute() ? element.AttributeIndex : element.StreamIndex;
                if (descriptorIndex != index) continue;
                selected = &element;
                break;
            }
            if (!selected) throw std::invalid_argument("Vertex declaration does not provide a shader descriptor");
            auto snapshot = Capture(selected->StreamIndex);
            if (layout.UsesDynamicVertexInput())
            {
                if (selected->Offset >= snapshot.Storage->Size() - snapshot.Offset) throw std::out_of_range("Vertex attribute offset exceeds its buffer");
                captured.push_back({index, std::move(snapshot.Storage), snapshot.Offset + selected->Offset, selected->Stride, VertexDeclaration::GetElementFormat(selected->Type), 1, selected->bUseInstanceIndex != 0});
            }
            else captured.push_back({index, std::move(snapshot.Storage), snapshot.Offset, selected->Stride});
        }
        return std::make_unique<VertexBufferTable>(mask, captured, layout.GetAddressHigh(), layout.UsesDynamicVertexInput());
    }
}
