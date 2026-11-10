#include "AnyUnreal/Process/Arguments.h"
#include "HAL/PlatformMisc.h"
#include "LaunchEngineLoop.h"
#include "Misc/CommandLine.h"
#include <vector>

extern int32 GuardedMain(const TCHAR* commandLine);

namespace
{
    struct ApplicationExit
    {
        ~ApplicationExit() { FEngineLoop::AppExit(); }
    };
}

int main()
{
    auto arguments = AnyUnreal::Process::Arguments::Read();
    std::vector<TCHAR*> pointers;
    pointers.reserve(arguments.size());
    for (auto& argument : arguments) pointers.push_back(argument.data());
    const FString commandLine = FCommandLine::BuildFromArgV(static_cast<const TCHAR*>(nullptr), static_cast<int32>(pointers.size()), pointers.data(), nullptr);
    int32 result = 0;
    {
        ApplicationExit exit;
        result = GuardedMain(*commandLine);
    }
    uint8 overriddenStatus = 0;
    if (FPlatformMisc::HasOverriddenReturnCode(&overriddenStatus)) result = overriddenStatus;
    return result;
}
