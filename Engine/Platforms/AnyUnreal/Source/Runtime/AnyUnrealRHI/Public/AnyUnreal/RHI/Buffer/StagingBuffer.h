#ifndef ANYUNREAL_RHI_STAGING_BUFFER_H
#define ANYUNREAL_RHI_STAGING_BUFFER_H

#include "AnyUnreal/RHI/Memory/Readback.h"
#include "HAL/CriticalSection.h"
#include "RHIResources.h"
#include <memory>

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API StagingBuffer final : public FRHIStagingBuffer
    {
    public:
        void* Lock(uint32 offset, uint32 size) override;
        void Unlock() override;
        uint64 GetGPUSizeBytes() const override;
        void Record(CommandBuffer& commands, const std::shared_ptr<Readback>& destination, std::span<const std::byte> source);

    private:
        mutable FCriticalSection mutex;
        std::shared_ptr<Readback> readback;
    };
}

#endif
