#ifndef UNREALPROSPERO_RHI_COMMAND_STORAGE_H
#define UNREALPROSPERO_RHI_COMMAND_STORAGE_H

#include "UnrealProspero/RHI/Memory/BufferAllocation.h"
#include "SceTypes.hpp"
#include <memory>

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API CommandStorage final
    {
    public:
        CommandStorage(std::size_t initialBytes, std::uint32_t addressHigh);
        CommandStorage(const CommandStorage&) = delete;
        CommandStorage& operator=(const CommandStorage&) = delete;
        CommandBuffer& Get();

    private:
        static bool APS5_VABI grow(CommandBuffer* commands, std::uint32_t requiredDwords, void* owner);
        void resize(std::uint32_t requiredDwords);
        std::uint32_t addressHigh;
        std::unique_ptr<BufferAllocation> allocation;
        CommandBuffer commands{};
    };
}

#endif
