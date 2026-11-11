#include "AnyUnreal/RHI/Descriptor/VertexBufferTable.h"
#include "AnyUnreal/RHI/Descriptor/BufferDescriptor.h"
#include <array>
#include <bit>
#include <stdexcept>

namespace AnyUnreal::RHI
{
    VertexBufferTable::VertexBufferTable(std::uint32_t usageMask, std::span<const VertexBufferBinding> bindings, std::uint32_t addressHigh)
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
        std::vector<std::array<std::uint32_t, 4>> descriptors;
        descriptors.reserve(bindings.size());
        sources.reserve(bindings.size());
        for (const auto* binding : indexed)
        {
            if (!binding) continue;
            const auto range = binding->Storage->GetRange(binding->Offset, binding->Storage->Size() - binding->Offset);
            descriptors.push_back(CreateVertexBufferDescriptor(range.data(), range.size(), binding->Stride));
            sources.push_back(binding->Storage);
        }
        if (!descriptors.empty())
        {
            const auto bytes = std::as_bytes(std::span(descriptors));
            table = std::make_unique<BufferAllocation>(bytes.size(), addressHigh);
            table->Upload(0, bytes);
        }
    }

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
