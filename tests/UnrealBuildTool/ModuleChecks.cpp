#include "UnrealProspero/Kernel/Module.h"
#include "UnrealProspero/Text/Encoding.h"
#include "HAL/PlatformProcess.h"
#include "UnrealProspero/Kernel/Error.h"
#include "prx/libkernel/include/Api.hpp"
#include <stdexcept>
#include <string>

bool CheckModules()
{
    using UnrealProspero::Kernel::Module;
    try
    {
        Module invalid("");
        return false;
    }
    catch (const std::invalid_argument&)
    {
    }
    const std::string executable(getargv_nid_postfix()[0]);
    const auto separator = executable.find_last_of("/\\");
    if (separator == std::string::npos) throw std::invalid_argument("Module check requires an executable path with a directory");
    const auto library = executable.substr(0, separator + 1) + "libs/libkernel.prx";
    Module first(library);
    Module second(library);
    using ThreadIdFunction = decltype(&scePthreadGetthreadid);
    auto threadId = reinterpret_cast<ThreadIdFunction>(first.FindSymbol("scePthreadGetthreadid"));
    if (threadId() != scePthreadGetthreadid()) return false;
    try
    {
        first.FindSymbol("UnrealProsperoMissingExportForModuleCheck");
        return false;
    }
    catch (const UnrealProspero::Kernel::Error&)
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
    void* handle = FPlatformProcess::GetDllHandle(UnrealProspero::Text::DecodeUtf8(library).c_str());
    threadId = reinterpret_cast<ThreadIdFunction>(FPlatformProcess::GetDllExport(handle, TEXT("scePthreadGetthreadid")));
    const bool passed = threadId() == scePthreadGetthreadid();
    FPlatformProcess::FreeDllHandle(handle);
    return passed;
}
