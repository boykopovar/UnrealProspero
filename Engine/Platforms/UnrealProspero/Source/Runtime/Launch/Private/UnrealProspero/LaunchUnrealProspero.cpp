#include "UnrealProspero/Process/Arguments.h"
#include "UnrealProspero/Process/Exit.h"
#include "HAL/PlatformMisc.h"
#include "LaunchEngineLoop.h"
#include "Misc/CommandLine.h"
#include <vector>
#if UNREALPROSPERO_TARGET_PS5
#include "Misc/FileHelper.h"
#include <stdexcept>
#endif

extern int32 GuardedMain(const TCHAR* commandLine);

namespace
{
    struct ApplicationExit
    {
        ~ApplicationExit() { FEngineLoop::AppExit(); }
    };

    int run()
    {
        auto arguments = UnrealProspero::Process::Arguments::Read();
        std::vector<TCHAR*> pointers;
        pointers.reserve(arguments.size());
        for (auto& argument : arguments) pointers.push_back(argument.data());
        const FString processCommandLine = FCommandLine::BuildFromArgV(static_cast<const TCHAR*>(nullptr), static_cast<int32>(pointers.size()), pointers.data(), nullptr);
#if UNREALPROSPERO_TARGET_PS5
        FString commandLine;
        if (!FFileHelper::LoadFileToString(commandLine, TEXT("/app0/UECommandLine.txt"))) throw std::runtime_error("Cannot read /app0/UECommandLine.txt");
        commandLine.TrimStartAndEndInline();
        if (commandLine.IsEmpty()) throw std::runtime_error("Empty /app0/UECommandLine.txt");
        if (!processCommandLine.IsEmpty())
        {
            commandLine += TEXT(" ");
            commandLine += processCommandLine;
        }
#else
        const FString& commandLine = processCommandLine;
#endif
        int32 result = 0;
        {
            ApplicationExit exit;
            result = GuardedMain(*commandLine);
        }
        uint8 overriddenStatus = 0;
        if (FPlatformMisc::HasOverriddenReturnCode(&overriddenStatus)) result = overriddenStatus;
        return result;
    }
}

int main()
{
    return UnrealProspero::Process::Exit::Complete(run());
}
