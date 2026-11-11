#ifndef UNREALPROSPERO_RHI_DESCRIPTOR_STORAGE_H
#define UNREALPROSPERO_RHI_DESCRIPTOR_STORAGE_H

#include "UnrealProspero/RHI/Memory/BufferAllocation.h"
#include "UnrealProspero/RHI/Command/MemoryAcquire.h"
#include "UnrealProspero/Shader/UserData.h"
#include <array>
#include <memory>

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API DescriptorStorage final
    {
    public:
        DescriptorStorage(const std::array<std::size_t, 4>& sizes, std::uint32_t addressHigh);
        void Upload(std::uint32_t set, std::size_t offset, std::span<const std::byte> source);
        std::span<const std::byte> GetRange(std::uint32_t set, std::size_t offset, std::size_t size) const;
        void Apply(Shader::UserDataInputs& inputs) const;
        std::vector<GpuMemoryRange> GetMemoryRanges() const;

    private:
        std::array<std::unique_ptr<BufferAllocation>, 4> tables;
    };
}

#endif
