#include "AnyUnreal/RHI/Memory/Readback.h"
#include "AnyUnreal/RHI/Command/BufferCopy.h"
#include "AnyUnreal/RHI/Command/MemoryAcquire.h"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    Readback::Readback(std::size_t size, std::uint32_t addressHigh) : storage(size, addressHigh), completion(addressHigh) {}

    void Readback::Record(CommandBuffer& commands, std::span<const std::byte> source)
    {
        if (recorded) throw std::runtime_error("Readback copy has already been recorded");
        if (source.size() != storage.Size() || !source.data()) throw std::invalid_argument("Readback source size does not match its storage");
        recorded = true;
        const GpuMemoryRange ranges[]{{source.data(), source.size()}, {storage.Data(), storage.Size()}};
        WriteMemoryAcquire(commands, ranges);
        WriteBufferCopy(commands, storage.Data(), source.data(), source.size());
        completion.Write(commands);
    }

    std::span<std::byte> Readback::GetRange(std::size_t offset, std::size_t size) const
    {
        if (!completion.IsComplete()) throw std::runtime_error("Readback copy has not completed on the GPU");
        return storage.GetRange(offset, size);
    }

    std::size_t Readback::Size() const { return storage.Size(); }
}
