#include "AnyUnreal/Process/Exit.h"
#include "HAL/PlatformMisc.h"
#include <charconv>
#include <cstdlib>
#include <stdexcept>
#include <string_view>

namespace
{
    void onExit()
    {
        std::_Exit(99);
    }

    struct ScopeExit
    {
        ~ScopeExit() { std::_Exit(98); }
    };
}

bool CheckExitStatus()
{
    using AnyUnreal::Process::Exit;
    uint8 status = 42;
    if (FPlatformMisc::HasOverriddenReturnCode(&status) || status != 42) return false;
    if (FPlatformMisc::HasOverriddenReturnCode(nullptr)) return false;
    for (const uint8 value : {0, 73, 255})
    {
        Exit::SetStatus(value);
        if (!FPlatformMisc::HasOverriddenReturnCode(&status) || status != value) return false;
        if (!FPlatformMisc::HasOverriddenReturnCode(nullptr)) return false;
    }
    return true;
}

[[noreturn]] void RunExitProbe(std::string_view argument)
{
    using AnyUnreal::Process::Exit;
    if (argument != "default")
    {
        unsigned int status = 0;
        const auto result = std::from_chars(argument.data(), argument.data() + argument.size(), status);
        if (result.ec != std::errc{} || result.ptr != argument.data() + argument.size() || status > 255) throw std::invalid_argument("Invalid exit probe status");
        Exit::SetStatus(static_cast<std::uint8_t>(status));
    }
    if (std::atexit(onExit) != 0) throw std::runtime_error("Cannot register exit probe handler");
    ScopeExit scope;
    Exit::Force();
}
