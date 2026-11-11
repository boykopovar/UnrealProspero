#ifndef UNREALPROSPERO_RHI_STAGING_BUFFER_H
#define UNREALPROSPERO_RHI_STAGING_BUFFER_H

#include "UnrealProspero/RHI/Memory/Readback.h"
#include "HAL/CriticalSection.h"
#include "RHIResources.h"
#include <memory>

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API StagingBuffer final : public FRHIStagingBuffer
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
