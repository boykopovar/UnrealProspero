#include "AnyUnreal/Kernel/Module.h"
#include "HAL/PlatformProcess.h"
#include "AnyUnreal/Kernel/Error.h"
#include "prx/libkernel/include/Api.hpp"
#include <stdexcept>

bool CheckModules()
{
    using AnyUnreal::Kernel::Module;
    try
    {
        Module invalid("");
        return false;
    }
    catch (const std::invalid_argument&)
    {
    }
    Module first("libs/libkernel.prx");
    Module second("libs/libkernel.prx");
    using ThreadIdFunction = decltype(&scePthreadGetthreadid);
    auto threadId = reinterpret_cast<ThreadIdFunction>(first.FindSymbol("scePthreadGetthreadid"));
    if (threadId() != scePthreadGetthreadid()) return false;
    try
    {
        first.FindSymbol("AnyUnrealMissingExportForModuleCheck");
        return false;
    }
    catch (const AnyUnreal::Kernel::Error&)
    {
    }
    first.Close();
    first.Close();
    try
    {
        first.FindSymbol("scePthreadGetthreadid");
        return false;
    }
    catch (const std::logic_error&)
    {
    }
    threadId = reinterpret_cast<ThreadIdFunction>(second.FindSymbol("scePthreadGetthreadid"));
    if (threadId() != scePthreadGetthreadid()) return false;
    second.Close();
    void* handle = FPlatformProcess::GetDllHandle(TEXT("libs/libkernel.prx"));
    threadId = reinterpret_cast<ThreadIdFunction>(FPlatformProcess::GetDllExport(handle, TEXT("scePthreadGetthreadid")));
    const bool passed = threadId() == scePthreadGetthreadid();
    FPlatformProcess::FreeDllHandle(handle);
    return passed;
}
