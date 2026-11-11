#ifndef ANYUNREAL_RHI_UNIFORM_BUFFER_H
#define ANYUNREAL_RHI_UNIFORM_BUFFER_H

#include "AnyUnreal/RHI/Buffer/UniformBufferStorage.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API UniformBuffer final : public FRHIUniformBuffer
    {
    public:
        UniformBuffer(const FRHIUniformBufferLayout* layout, const void* contents, EUniformBufferUsage usage, EUniformBufferValidation validation, std::uint32_t addressHigh);
        void Update(const void* contents);
        std::shared_ptr<const UniformBufferStorage> Capture() const;

    private:
        EUniformBufferValidation validation;
        std::uint32_t addressHigh;
        std::shared_ptr<const UniformBufferStorage> storage;
    };
}

#endif
