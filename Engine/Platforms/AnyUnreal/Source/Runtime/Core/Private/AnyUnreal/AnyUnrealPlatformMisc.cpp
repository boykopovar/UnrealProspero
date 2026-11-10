#include "AnyUnreal/Kernel/SystemConfiguration.h"
#include "AnyUnreal/AnyUnrealPlatformMisc.h"
#include "AnyUnreal/Kernel/Debug.h"
#include "AnyUnreal/Text/Encoding.h"
#include "prx/libkernel/File/include/File.hpp"
#include <atomic>
#include <stdexcept>

bool FAnyUnrealPlatformMisc::IsDebuggerPresent()
{
    throw std::runtime_error("Debugger detection is not implemented for AnyUnreal");
}

void FAnyUnrealPlatformMisc::DebugBreak()
{
    AnyUnreal::Kernel::DebugBreak();
}

void FAnyUnrealPlatformMisc::PromptForRemoteDebugging(bool)
{
    throw std::runtime_error("Remote debugger attachment is not implemented");
}

void FAnyUnrealPlatformMisc::MemoryBarrier()
{
    std::atomic_thread_fence(std::memory_order_seq_cst);
}

void FAnyUnrealPlatformMisc::LowLevelOutputDebugString(const TCHAR* message)
{
    if (message == nullptr) throw std::invalid_argument("Null debug message");
    std::string text = AnyUnreal::Text::EncodeUtf8(message);
    std::size_t offset = 0;
    while (offset < text.size())
    {
        std::int64_t written = sceKernelWrite(2, text.data() + offset, text.size() - offset);
        if (written <= 0 || static_cast<std::uint64_t>(written) > text.size() - offset) throw std::runtime_error("Debug output write failed: " + std::to_string(written));
        offset += static_cast<std::size_t>(written);
    }
}

int32 FAnyUnrealPlatformMisc::NumberOfCoresIncludingHyperthreads()
{
    return AnyUnreal::Kernel::SystemConfiguration::LogicalProcessorCount();
}
