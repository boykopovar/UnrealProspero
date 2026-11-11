#include "AnyUnreal/RHI/Buffer/StagingBuffer.h"
#include "Misc/ScopeLock.h"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    void* StagingBuffer::Lock(uint32 offset, uint32 size)
    {
        FScopeLock lock(&mutex);
        if (bIsLocked || !readback) throw std::runtime_error("Staging buffer is locked or has no readback copy");
        auto range = readback->GetRange(offset, size);
        bIsLocked = true;
        return range.data();
    }

    void StagingBuffer::Unlock()
    {
        FScopeLock lock(&mutex);
        if (!bIsLocked) throw std::runtime_error("Staging buffer is not locked");
        bIsLocked = false;
    }

    uint64 StagingBuffer::GetGPUSizeBytes() const
    {
        FScopeLock lock(&mutex);
        return readback ? readback->Size() : 0;
    }

    void StagingBuffer::Record(CommandBuffer& commands, const std::shared_ptr<Readback>& destination, std::span<const std::byte> source)
    {
        FScopeLock lock(&mutex);
        if (bIsLocked || !destination) throw std::invalid_argument("Staging copy requires an unlocked buffer and readback storage");
        destination->Record(commands, source);
        readback = destination;
    }
}
