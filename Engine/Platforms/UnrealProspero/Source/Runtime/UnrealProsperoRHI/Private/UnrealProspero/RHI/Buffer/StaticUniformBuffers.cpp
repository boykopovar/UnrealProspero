#include "UnrealProspero/RHI/Buffer/StaticUniformBuffers.h"
#include "RHICoreShader.h"
#include <stdexcept>

namespace UnrealProspero::RHI
{
    void StaticUniformBuffers::Set(FUniformBufferStaticSlot slot, FRHIUniformBuffer* buffer)
    {
        if (!IsUniformBufferStaticSlotValid(slot)) throw std::out_of_range("Invalid static uniform buffer slot");
        if (buffer && buffer->GetLayout().StaticSlot != slot) throw std::invalid_argument("Uniform buffer layout does not match its static slot");
        buffers[slot] = buffer;
    }

    void StaticUniformBuffers::Replace(const FUniformBufferStaticBindings& bindings)
    {
        StaticUniformBuffers replacement;
        std::array<bool, MAX_UNIFORM_BUFFER_STATIC_SLOTS> assigned{};
        for (int32 index = 0; index < bindings.GetUniformBufferCount(); ++index)
        {
            const auto slot = bindings.GetSlot(index);
            if (!IsUniformBufferStaticSlotValid(slot)) throw std::out_of_range("Invalid static uniform buffer slot");
            if (assigned[slot]) throw std::invalid_argument("Duplicate static uniform buffer slot");
            replacement.Set(slot, bindings.GetUniformBuffer(index));
            assigned[slot] = true;
        }
        buffers.swap(replacement.buffers);
    }

    std::vector<FRHIShaderParameterResource> StaticUniformBuffers::Resolve(const FRHIShader& shader) const
    {
        const auto& slots = shader.GetStaticSlots();
        const auto& hashes = shader.GetShaderResourceTable().ResourceTableLayoutHashes;
        if (slots.Num() != hashes.Num()) throw std::invalid_argument("Shader static slots do not match its uniform buffer layouts");
        std::vector<FRHIShaderParameterResource> result;
        for (int32 index = 0; index < slots.Num(); ++index)
        {
            const auto slot = slots[index];
            if (!IsUniformBufferStaticSlotValid(slot)) continue;
            auto* buffer = buffers[slot].GetReference();
            UE::RHICore::ValidateStaticUniformBuffer(buffer, slot, hashes[index]);
            if (!buffer) continue;
            if (buffer->GetLayout().GetHash() != hashes[index]) throw std::invalid_argument("Static uniform buffer layout does not match the shader");
            if (index > 65535) throw std::out_of_range("Static uniform buffer index exceeds the shader parameter index");
            result.emplace_back(FRHIShaderParameterResource::EType::UniformBuffer, buffer, static_cast<uint16>(index));
        }
        return result;
    }
}
