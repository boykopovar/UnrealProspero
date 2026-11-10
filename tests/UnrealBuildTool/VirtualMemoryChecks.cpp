#include "HAL/PlatformMemory.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include <cstring>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <stdexcept>

namespace
{
    bool isCommitted(void* address)
    {
        VirtualQueryInfo info{};
        AnyUnreal::Kernel::CheckResult(sceKernelVirtualQuery(address, 0, &info, sizeof(info)), "sceKernelVirtualQuery");
        return info.is_committed != 0;
    }
}

bool CheckVirtualMemory()
{
    using Block = FPlatformMemory::FPlatformVirtualMemoryBlock;
    constexpr auto page = Block::GetCommitAlignment();
    for (const auto alignment : {std::size_t{16}, page, std::size_t{65536}, std::size_t{2097152}})
    {
        auto block = Block::AllocateVirtual(page * 3 - 1, alignment);
        struct Cleanup
        {
            Block& Value;
            ~Cleanup() noexcept(false) { Value.FreeVirtual(); }
        } cleanup{block};
        auto* bytes = static_cast<unsigned char*>(block.GetVirtualPointer());
        if (block.GetActualSize() != page * 3 || reinterpret_cast<std::uintptr_t>(bytes) % alignment != 0 || isCommitted(bytes)) return false;
        block.Commit(page, page);
        if (isCommitted(bytes) || !isCommitted(bytes + page) || isCommitted(bytes + page * 2)) return false;
        std::memset(bytes + page, 0x5a, page);
        block.Commit();
        for (std::size_t i = 0; i != page * 3; ++i) if (bytes[i] != (i >= page && i < page * 2 ? 0x5a : 0)) return false;
        block.DecommitByPtr(bytes + page, page);
        if (!isCommitted(bytes) || isCommitted(bytes + page) || !isCommitted(bytes + page * 2)) return false;
        block.DecommitByPtr(bytes + page, page);
        block.CommitByPtr(bytes + page, page);
        for (std::size_t i = page; i != page * 2; ++i) if (bytes[i] != 0) return false;
        try
        {
            block.Commit(page * 3, page);
            return false;
        }
        catch (const std::invalid_argument&)
        {
        }
        try
        {
            block.Decommit(1, page);
            return false;
        }
        catch (const std::invalid_argument&)
        {
        }
        block.Commit(page * 3, 0);
        block.Decommit();
        if (isCommitted(bytes)) return false;
        block.FreeVirtual();
        block.FreeVirtual();
        if (block.GetVirtualPointer() || block.GetActualSize() != 0) return false;
        try
        {
            block.Commit();
            return false;
        }
        catch (const std::logic_error&)
        {
        }
    }
    {
        auto original = Block::AllocateVirtual(page * 3);
        original.Commit(page, page);
        Block restored(original.GetVirtualPointer(), original.GetActualSizeInPages());
        if (restored.GetActualSize() != original.GetActualSize()) return false;
        restored.FreeVirtual();
        original = Block{};
    }
    for (const auto size : {std::size_t{0}, std::numeric_limits<std::size_t>::max()})
    {
        try
        {
            auto block = Block::AllocateVirtual(size);
            block.FreeVirtual();
            return false;
        }
        catch (const std::invalid_argument&)
        {
        }
    }
    return true;
}
