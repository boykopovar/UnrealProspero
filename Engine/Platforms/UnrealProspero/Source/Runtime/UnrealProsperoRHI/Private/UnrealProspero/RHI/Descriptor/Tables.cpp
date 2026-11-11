#include "UnrealProspero/RHI/Descriptor/Tables.h"
#include "UnrealProspero/RHI/Descriptor/BufferDescriptor.h"
#include <algorithm>
#include <stdexcept>

namespace UnrealProspero::RHI
{
    namespace
    {
        std::array<std::size_t, 4> storageSizes(std::span<const Shader::RuntimeBinding> bindings)
        {
            Shader::ValidateRuntimeBindings(bindings);
            std::array<std::size_t, 4> sizes{};
            for (const auto& binding : bindings) sizes[binding.Set] = std::max(sizes[binding.Set], static_cast<std::size_t>(binding.Offset) + static_cast<std::size_t>(binding.Count) * binding.Stride);
            return sizes;
        }
    }

    DescriptorTables::DescriptorTables(std::span<const Shader::RuntimeBinding> bindings, std::uint32_t addressHigh) : storage(storageSizes(bindings), addressHigh)
    {
        entries.reserve(bindings.size());
        for (const auto& binding : bindings) entries.push_back({binding, std::vector<bool>(binding.Count)});
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
        storage.Upload(binding.Set, binding.Offset + static_cast<std::size_t>(element) * binding.Stride, descriptor);
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
        return storage.GetMemoryRanges();
    }

    void DescriptorTables::validateInitialized() const
    {
        for (const auto& entry : entries)
        {
            if (std::find(entry.initialized.begin(), entry.initialized.end(), false) != entry.initialized.end()) throw std::runtime_error("Shader descriptor binding is incomplete");
        }
    }

    std::vector<DescriptorRange> DescriptorTables::GetRanges() const
    {
        validateInitialized();
        std::vector<DescriptorRange> result;
        result.reserve(entries.size());
        for (const auto& entry : entries)
        {
            const auto& binding = entry.binding;
            result.push_back({binding.Set, binding.Binding, binding.Offset, storage.GetRange(binding.Set, binding.Offset, static_cast<std::size_t>(binding.Count) * binding.Stride)});
        }
        return result;
    }

    void DescriptorTables::Apply(Shader::UserDataInputs& inputs) const
    {
        validateInitialized();
        storage.Apply(inputs);
    }
}
