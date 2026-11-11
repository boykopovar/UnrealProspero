#ifndef ANYUNREAL_RHI_UNIFORM_BUFFER_STORAGE_H
#define ANYUNREAL_RHI_UNIFORM_BUFFER_STORAGE_H

#include "AnyUnreal/RHI/Memory/BufferAllocation.h"
#include "RHIResources.h"
#include <memory>

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API UniformBufferStorage final
    {
    public:
        UniformBufferStorage(const FRHIUniformBufferLayout& layout, const void* contents, EUniformBufferValidation validation, std::uint32_t addressHigh);
        const BufferAllocation* GetConstants() const;
        const TArray<TRefCountPtr<FRHIResource>>& GetResources() const;

    private:
        std::unique_ptr<BufferAllocation> constants;
        TArray<TRefCountPtr<FRHIResource>> resources;
    };
}

#endif
