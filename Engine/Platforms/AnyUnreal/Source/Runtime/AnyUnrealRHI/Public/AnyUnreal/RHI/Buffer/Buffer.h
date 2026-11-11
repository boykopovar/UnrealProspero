#ifndef ANYUNREAL_RHI_BUFFER_H
#define ANYUNREAL_RHI_BUFFER_H

#include "AnyUnreal/RHI/Memory/BufferAllocation.h"
#include "RHIResources.h"
#include <memory>

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API Buffer final : public FRHIBuffer
    {
    public:
        Buffer(const FRHIBufferCreateDesc& description, std::uint32_t addressHigh);
        std::shared_ptr<BufferAllocation> Capture() const;

    private:
        std::shared_ptr<BufferAllocation> storage;
    };
}

#endif
