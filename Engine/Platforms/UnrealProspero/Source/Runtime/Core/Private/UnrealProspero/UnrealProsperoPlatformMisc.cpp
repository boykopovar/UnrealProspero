#include "UnrealProspero/UnrealProsperoPlatformMisc.h"
#include "UnrealProspero/UnrealProsperoPlatformProcess.h"
#include "UnrealProspero/Kernel/SystemConfiguration.h"
#include "UnrealProspero/Kernel/Debug.h"
#include "UnrealProspero/Text/Encoding.h"
#include "prx/libkernel/File/include/File.hpp"
#include <atomic>
#include <stdexcept>
#include <string>

bool FUnrealProsperoPlatformMisc::IsDebuggerPresent()
{
#if UNREALPROSPERO_TARGET_PS5
    return UnrealProspero::Kernel::IsDebuggerAttached();
#else
    throw std::runtime_error("Debugger detection is not implemented for UnrealProspero");
#endif
}

void FUnrealProsperoPlatformMisc::DebugBreak()
{
    UnrealProspero::Kernel::DebugBreak();
}

void FUnrealProsperoPlatformMisc::PromptForRemoteDebugging(bool)
{
    throw std::runtime_error("Remote debugger attachment is not implemented");
}

void FUnrealProsperoPlatformMisc::MemoryBarrier()
{
    std::atomic_thread_fence(std::memory_order_seq_cst);
}

void FUnrealProsperoPlatformMisc::LowLevelOutputDebugString(const TCHAR* message)
{
    if (message == nullptr) throw std::invalid_argument("Null debug message");
    std::string text = UnrealProspero::Text::EncodeUtf8(message);
    std::size_t offset = 0;
    while (offset < text.size())
    {
        std::int64_t written = sceKernelWrite(2, text.data() + offset, text.size() - offset);
        if (written <= 0 || static_cast<std::uint64_t>(written) > text.size() - offset) throw std::runtime_error("Debug output write failed: " + std::to_string(written));
        offset += static_cast<std::size_t>(written);
    }
}

int32 FUnrealProsperoPlatformMisc::NumberOfCoresIncludingHyperthreads()
{
    return UnrealProspero::Kernel::SystemConfiguration::LogicalProcessorCount();
}

#if UNREALPROSPERO_TARGET_PS5
const TCHAR* FUnrealProsperoPlatformMisc::RootDir()
{
    return FUnrealProsperoPlatformProcess::BaseDir();
}

const TCHAR* FUnrealProsperoPlatformMisc::EngineDir()
{
    static const auto directory = std::basic_string<TCHAR>(RootDir()) + TEXT("Engine/");
    return directory.c_str();
}
#endif
