#include "AnyUnreal/RHI/Command/CommandStorage.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace AnyUnreal::RHI
{
    CommandStorage::CommandStorage(std::size_t initialBytes, std::uint32_t addressHigh) : addressHigh(addressHigh)
    {
        if (initialBytes == 0 || initialBytes % sizeof(std::uint32_t) != 0) throw std::invalid_argument("Command buffer capacity must be a positive number of dwords");
        allocation = std::make_unique<BufferAllocation>(initialBytes, addressHigh);
        auto* words = static_cast<std::uint32_t*>(allocation->Data());
        commands = {words, words + initialBytes / sizeof(std::uint32_t), words, words + initialBytes / sizeof(std::uint32_t), grow, this, 0};
    }

    CommandBuffer& CommandStorage::Get() { return commands; }

    bool APS5_VABI CommandStorage::grow(CommandBuffer* commands, std::uint32_t requiredDwords, void* owner)
    {
        if (!owner) throw std::invalid_argument("Command buffer growth requires an owner");
        auto& storage = *static_cast<CommandStorage*>(owner);
        if (commands != &storage.commands) throw std::invalid_argument("Command buffer does not belong to its storage");
        storage.resize(requiredDwords);
        return true;
    }

    void CommandStorage::resize(std::uint32_t requiredDwords)
    {
        if (commands.cursor_down != commands.top) throw std::runtime_error("Command buffer growth cannot relocate embedded data");
        const auto used = static_cast<std::size_t>(commands.cursor_up - commands.bottom) * sizeof(std::uint32_t);
        const auto required = static_cast<std::size_t>(requiredDwords) * sizeof(std::uint32_t);
        constexpr auto limit = static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) - Kernel::DirectAllocation::PageSize + 1;
        if (used > limit || required > limit - used) throw std::length_error("Command buffer exceeds its address aperture");
        const auto capacity = std::max(used + required, std::min(limit, allocation->Size() * 2));
        auto replacement = std::make_unique<BufferAllocation>(capacity, addressHigh);
        if (used != 0) replacement->Upload(0, allocation->GetRange(0, used));
        auto* words = static_cast<std::uint32_t*>(replacement->Data());
        commands.bottom = words;
        commands.top = words + capacity / sizeof(std::uint32_t);
        commands.cursor_up = words + used / sizeof(std::uint32_t);
        commands.cursor_down = commands.top;
        allocation = std::move(replacement);
    }
}
