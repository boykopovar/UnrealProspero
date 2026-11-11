#include "UnrealProspero/RHI/Descriptor/VertexBufferTable.h"
#include "UnrealProspero/RHI/Descriptor/BufferDescriptor.h"
#include <array>
#include <bit>
#include <stdexcept>

namespace UnrealProspero::RHI
{
    VertexBufferTable::VertexBufferTable(std::uint32_t usageMask, std::span<const VertexBufferBinding> bindings, std::uint32_t addressHigh, bool dynamic) : usageMask(usageMask)
    {
        if (bindings.size() != static_cast<std::size_t>(std::popcount(usageMask))) throw std::invalid_argument("Vertex bindings do not match the shader usage mask");
        std::array<const VertexBufferBinding*, 32> indexed{};
        for (const auto& binding : bindings)
        {
            if (binding.Index >= indexed.size() || (usageMask & (std::uint32_t{1} << binding.Index)) == 0) throw std::invalid_argument("Unexpected vertex descriptor index");
            if (indexed[binding.Index]) throw std::invalid_argument("Duplicate vertex descriptor index");
            if (!binding.Storage || binding.Offset >= binding.Storage->Size()) throw std::invalid_argument("Vertex descriptor requires a valid buffer range");
            indexed[binding.Index] = &binding;
        }
        std::vector<std::uint32_t> descriptors;
        descriptors.reserve(bindings.size() * (dynamic ? 8 : 4));
        sources.reserve(bindings.size());
        for (const auto* binding : indexed)
        {
            if (!binding) continue;
            const auto range = binding->Storage->GetRange(binding->Offset, binding->Storage->Size() - binding->Offset);
            if (!dynamic && (binding->Format != BufferFormat::Untyped || binding->InstanceRate || binding->InstanceDivisor != 1)) throw std::invalid_argument("Dynamic attributes supplied to a static vertex table");
            const auto descriptor = dynamic ? CreateTypedVertexBufferDescriptor(range.data(), range.size(), binding->Stride, binding->Format) : CreateVertexBufferDescriptor(range.data(), range.size(), binding->Stride);
            descriptors.insert(descriptors.end(), descriptor.begin(), descriptor.end());
            if (dynamic) descriptors.insert(descriptors.end(), {binding->InstanceDivisor, binding->InstanceRate ? 1u : 0u, 0u, 0u});
            sources.push_back(binding->Storage);
        }
        if (!descriptors.empty())
        {
            const auto bytes = std::as_bytes(std::span(descriptors));
            table = std::make_unique<BufferAllocation>(bytes.size(), addressHigh);
            table->Upload(0, bytes);
        }
    }

    std::uint32_t VertexBufferTable::GetUsageMask() const { return usageMask; }

    void VertexBufferTable::Apply(Shader::UserDataInputs& inputs) const
    {
        inputs.VertexBuffers = table ? reinterpret_cast<std::uintptr_t>(table->Data()) : 0;
    }

    std::vector<GpuMemoryRange> VertexBufferTable::GetMemoryRanges() const
    {
        std::vector<GpuMemoryRange> ranges;
        ranges.reserve(sources.size() + (table ? 1 : 0));
        for (const auto& source : sources) ranges.push_back({source->Data(), source->Size()});
        if (table) ranges.push_back({table->Data(), table->Size()});
        return ranges;
    }
}
