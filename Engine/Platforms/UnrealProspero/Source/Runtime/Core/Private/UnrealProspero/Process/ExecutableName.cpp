#include "UnrealProspero/UnrealProsperoPlatformProcess.h"
#include "UnrealProspero/Process/ExecutablePath.h"
#include "UnrealProspero/Text/Encoding.h"
#include <array>
#include <stdexcept>

const TCHAR* FUnrealProsperoPlatformProcess::ExecutableName(bool removeExtension)
{
    static const auto names = []
    {
        const auto path = UnrealProspero::Process::ExecutablePath();
        const auto separator = path.find_last_of('/');
        auto name = path.substr(separator == std::string::npos ? 0 : separator + 1);
        if (name.empty()) throw std::runtime_error("Executable path has no filename");
        const auto full = UnrealProspero::Text::DecodeUtf8(name);
        const auto extension = name.find_last_of('.');
        if (extension != std::string::npos && extension != 0) name.resize(extension);
        return std::array{full, UnrealProspero::Text::DecodeUtf8(name)};
    }();
    return names[removeExtension ? 1 : 0].c_str();
}
