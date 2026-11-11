#ifndef UNREALPROSPERO_RHI_UNIFORM_BUFFER_H
#define UNREALPROSPERO_RHI_UNIFORM_BUFFER_H

#include "UnrealProspero/RHI/Buffer/UniformBufferStorage.h"

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API UniformBuffer final : public FRHIUniformBuffer
    {
    public:
        UniformBuffer(const FRHIUniformBufferLayout* layout, const void* contents, EUniformBufferUsage usage, EUniformBufferValidation validation, std::uint32_t addressHigh);
        void Update(const void* contents);
        void Update(FRHICommandListBase& commandList, const void* contents);
        std::shared_ptr<const UniformBufferStorage> Capture() const;

    private:
        std::shared_ptr<const UniformBufferStorage> prepareUpdate(const void* contents) const;
        void applyUpdate(std::shared_ptr<const UniformBufferStorage> replacement);
        EUniformBufferValidation validation;
        std::uint32_t addressHigh;
        std::shared_ptr<const UniformBufferStorage> storage;
    };
}

#endif
