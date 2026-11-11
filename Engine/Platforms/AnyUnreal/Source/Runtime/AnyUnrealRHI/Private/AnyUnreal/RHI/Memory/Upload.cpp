#include "AnyUnreal/RHI/Memory/Upload.h"
#include "AnyUnreal/RHI/Command/BufferCopy.h"
#include "AnyUnreal/RHI/Command/MemoryAcquire.h"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    Upload::Upload(std::span<const std::byte> source, std::uint32_t addressHigh) : storage(source.size(), addressHigh), barrier(addressHigh)
    {
        if (!source.data()) throw std::invalid_argument("Buffer upload source is null");
        storage.Upload(0, source);
    }

    void Upload::Record(CommandBuffer& commands, std::span<std::byte> destination)
    {
        if (recorded) throw std::runtime_error("Buffer upload has already been recorded");
        if (!destination.data() || destination.size() != storage.Size()) throw std::invalid_argument("Buffer upload destination does not match the source size");
        recorded = true;
        const GpuMemoryRange ranges[]{{storage.Data(), storage.Size()}, {destination.data(), destination.size()}};
        barrier.Release(commands);
        barrier.Acquire(commands, ranges);
        WriteBufferCopy(commands, destination.data(), storage.Data(), storage.Size());
    }
}
