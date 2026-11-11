#ifndef ANYUNREAL_RHI_BUFFER_H
#define ANYUNREAL_RHI_BUFFER_H

#include "AnyUnreal/RHI/Memory/BufferAllocation.h"
#include "RHIResources.h"
#include "HAL/CriticalSection.h"
#include <memory>
#include <optional>
#include <vector>

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API Buffer final : public FRHIBuffer
    {
    public:
        struct LockedRange
        {
            std::size_t Offset;
            EResourceLockMode Mode;
            std::vector<std::byte> Data;
        };

        Buffer(const FRHIBufferCreateDesc& description, std::uint32_t addressHigh);
        std::shared_ptr<BufferAllocation> Capture() const;
        void* Lock(std::size_t offset, std::size_t size, EResourceLockMode mode);
        LockedRange Unlock();

    private:
        friend class BufferInitializer;
        enum class InitializationState
        {
            Ready,
            Pending,
            Writing
        };

        void* beginInitialization();
        void finishInitialization();
        InitializationState initializationState = InitializationState::Ready;
        std::shared_ptr<BufferAllocation> storage;
        FCriticalSection lockMutex;
        std::optional<LockedRange> lockedRange;
    };
}

#endif
