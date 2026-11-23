#include "UnrealProspero/Process/Arguments.h"
#include "UnrealProspero/Process/Exit.h"
#include "HAL/PlatformMisc.h"
#include "LaunchEngineLoop.h"
#include "Misc/CommandLine.h"
#include <vector>
#if UNREALPROSPERO_TARGET_PS5
#include "GenericPlatform/GenericPlatformFile.h"
#include "Misc/FileHelper.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
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
        std::unique_ptr<IFileHandle> file(IPlatformFile::GetPlatformPhysical().OpenRead(TEXT("/app0/UECommandLine.txt")));
        if (!file) throw std::runtime_error("Cannot open /app0/UECommandLine.txt");
        const auto size = file->Size();
        if (size <= 0 || size > std::numeric_limits<int32>::max()) throw std::runtime_error("Invalid /app0/UECommandLine.txt size");
        std::vector<uint8> bytes(static_cast<std::size_t>(size));
        if (!file->Read(bytes.data(), size)) throw std::runtime_error("Cannot read /app0/UECommandLine.txt");
        file.reset();
        FString commandLine;
        FFileHelper::BufferToString(commandLine, bytes.data(), static_cast<int32>(size));
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
#if UNREALPROSPERO_TARGET_PS5
    try
    {
        return UnrealProspero::Process::Exit::Complete(run());
    }
    catch (const std::exception& error)
    {
        const std::string message = std::string("UnrealProspero startup error: ") + error.what() + '\n';
        UnrealProspero::Kernel::CheckResult(sceKernelDebugOutText(0, message.c_str()), "sceKernelDebugOutText");
        throw;
    }
#else
    return UnrealProspero::Process::Exit::Complete(run());
#endif
}
