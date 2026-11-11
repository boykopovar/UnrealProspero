#include "AnyUnreal/RHI/Buffer/UniformBuffer.h"
#include "RHICommandList.h"
#include <stdexcept>
#include <utility>

namespace AnyUnreal::RHI
{
    namespace
    {
        const FRHIUniformBufferLayout* checkedLayout(const FRHIUniformBufferLayout* layout, EUniformBufferUsage usage)
        {
            if (!layout) throw std::invalid_argument("Uniform buffer layout is null");
            if (usage != UniformBuffer_SingleDraw && usage != UniformBuffer_SingleFrame && usage != UniformBuffer_MultiFrame) throw std::invalid_argument("Invalid uniform buffer usage");
            return layout;
        }
    }

    UniformBuffer::UniformBuffer(const FRHIUniformBufferLayout* layout, const void* contents, EUniformBufferUsage usage, EUniformBufferValidation validation, std::uint32_t addressHigh) : FRHIUniformBuffer(checkedLayout(layout, usage)), validation(validation), addressHigh(addressHigh), storage(std::make_shared<UniformBufferStorage>(*layout, contents, validation, addressHigh))
    {
        ResourceTable = storage->GetResources();
    }

    std::shared_ptr<const UniformBufferStorage> UniformBuffer::prepareUpdate(const void* contents) const
    {
        if (!contents) throw std::invalid_argument("Uniform buffer update contents are null");
        return std::make_shared<UniformBufferStorage>(GetLayout(), contents, validation, addressHigh);
    }

    void UniformBuffer::applyUpdate(std::shared_ptr<const UniformBufferStorage> replacement)
    {
        ResourceTable = replacement->GetResources();
        storage = std::move(replacement);
    }

    void UniformBuffer::Update(const void* contents)
    {
        applyUpdate(prepareUpdate(contents));
    }

    void UniformBuffer::Update(FRHICommandListBase& commandList, const void* contents)
    {
        auto replacement = prepareUpdate(contents);
        commandList.EnqueueLambda([buffer = TRefCountPtr<UniformBuffer>(this), replacement = std::move(replacement)](FRHICommandListBase&) mutable
        {
            buffer->applyUpdate(std::move(replacement));
        });
    }

    std::shared_ptr<const UniformBufferStorage> UniformBuffer::Capture() const { return storage; }
}
