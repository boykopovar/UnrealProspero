#include "AnyUnreal/RHI/Memory/Upload.h"
#include "AnyUnreal/RHI/Command/BufferCopy.h"
#include "AnyUnreal/RHI/Command/MemoryAcquire.h"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    Upload::Upload(std::span<const std::byte> source, std::uint32_t addressHigh) : storage(source.size(), addressHigh)
    {
        if (!source.data()) throw std::invalid_argument("Buffer upload source is null");
        storage.Upload(0, source);
    }

    void Upload::Record(CommandBuffer& commands, std::span<std::byte> destination) const
    {
        if (!destination.data() || destination.size() != storage.Size()) throw std::invalid_argument("Buffer upload destination does not match the source size");
        const GpuMemoryRange ranges[]{{storage.Data(), storage.Size()}, {destination.data(), destination.size()}};
        WriteMemoryAcquire(commands, ranges);
        WriteBufferCopy(commands, destination.data(), storage.Data(), storage.Size());
    }
}
