#include "AnyUnreal/RHI/Descriptor/Tables.h"
#include "AnyUnreal/RHI/Descriptor/BufferDescriptor.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace AnyUnreal::RHI
{
    DescriptorTables::DescriptorTables(std::span<const Shader::RuntimeBinding> bindings, std::uint32_t addressHigh)
    {
        Shader::ValidateRuntimeBindings(bindings);
        std::array<std::size_t, 4> sizes{};
        entries.reserve(bindings.size());
        for (const auto& binding : bindings)
        {
            sizes[binding.Set] = std::max(sizes[binding.Set], static_cast<std::size_t>(binding.Offset) + static_cast<std::size_t>(binding.Count) * binding.Stride);
            entries.push_back({binding, std::vector<bool>(binding.Count)});
        }
        for (std::size_t index = 0; index < tables.size(); ++index)
        {
            if (sizes[index] != 0) tables[index] = std::make_unique<BufferAllocation>(sizes[index], addressHigh);
        }
    }

    DescriptorTables::Entry& DescriptorTables::findEntry(Shader::ResourceKind kind, std::uint32_t parameterIndex)
    {
        for (auto& entry : entries)
        {
            if (entry.binding.Kind == kind && parameterIndex >= entry.binding.ParameterIndex && parameterIndex - entry.binding.ParameterIndex < entry.binding.Count) return entry;
        }
        throw std::invalid_argument("Shader parameter has no descriptor binding");
    }

    void DescriptorTables::SetDescriptor(Shader::ResourceKind kind, std::uint32_t parameterIndex, std::span<const std::byte> descriptor)
    {
        auto& entry = findEntry(kind, parameterIndex);
        const auto& binding = entry.binding;
        if (descriptor.size() != binding.Stride) throw std::invalid_argument("Descriptor size does not match the shader binding");
        const auto element = parameterIndex - binding.ParameterIndex;
        auto* destination = static_cast<std::byte*>(tables[binding.Set]->Data()) + binding.Offset + static_cast<std::size_t>(element) * binding.Stride;
        std::memcpy(destination, descriptor.data(), descriptor.size());
        entry.initialized[element] = true;
    }

    void DescriptorTables::SetBuffer(Shader::ResourceKind kind, std::uint32_t parameterIndex, const void* address, std::size_t size)
    {
        const auto& binding = findEntry(kind, parameterIndex).binding;
        if (binding.Type != "uniform-buffer" && binding.Type != "storage-buffer") throw std::invalid_argument("Shader parameter is not a byte-address buffer");
        if (size < binding.BlockSize) throw std::invalid_argument("Uniform buffer is smaller than the shader block");
        const auto descriptor = CreateByteAddressBufferDescriptor(address, size);
        SetDescriptor(kind, parameterIndex, std::as_bytes(std::span(descriptor)));
    }

    std::vector<GpuMemoryRange> DescriptorTables::GetMemoryRanges() const
    {
        std::vector<GpuMemoryRange> result;
        for (const auto& table : tables)
        {
            if (table) result.push_back({table->Data(), table->Size()});
        }
        return result;
    }

    void DescriptorTables::Apply(Shader::UserDataInputs& inputs) const
    {
        for (const auto& entry : entries)
        {
            if (std::find(entry.initialized.begin(), entry.initialized.end(), false) != entry.initialized.end()) throw std::runtime_error("Shader descriptor binding is incomplete");
        }
        for (std::size_t index = 0; index < tables.size(); ++index) inputs.DescriptorSets[index] = tables[index] ? reinterpret_cast<std::uintptr_t>(tables[index]->Data()) : 0;
    }
}
