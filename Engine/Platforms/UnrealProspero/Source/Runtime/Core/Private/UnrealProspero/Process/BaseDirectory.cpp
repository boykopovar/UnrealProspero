#include "UnrealProspero/UnrealProsperoPlatformProcess.h"
#include "UnrealProspero/Process/ExecutablePath.h"
#include "UnrealProspero/Text/Encoding.h"
#include <string>

const TCHAR* FUnrealProsperoPlatformProcess::BaseDir()
{
    static const auto directory = []
    {
        auto value = UnrealProspero::Process::ExecutablePath();
        const auto separator = value.find_last_of('/');
        if (separator == std::string::npos) throw std::runtime_error("Executable path has no directory");
        value.resize(separator + 1);
        return UnrealProspero::Text::DecodeUtf8(value);
    }();
    return directory.c_str();
}
