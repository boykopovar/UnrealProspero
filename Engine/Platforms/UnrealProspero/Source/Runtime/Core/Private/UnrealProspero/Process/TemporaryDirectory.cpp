#include "UnrealProspero/UnrealProsperoPlatformProcess.h"
#include "UnrealProspero/File/TemporaryStorage.h"
#include "UnrealProspero/Text/Encoding.h"
#include <string>
#if UNREALPROSPERO_TARGET_PS5
#include "Containers/UnrealString.h"
#endif

namespace
{
    struct TemporaryDirectory final
    {
        UnrealProspero::File::TemporaryStorage Storage;
        std::basic_string<TCHAR> Path;

        TemporaryDirectory() : Path(UnrealProspero::Text::DecodeUtf8(Storage.GetPath()))
        {
            if (Path.back() != TEXT('/')) Path.push_back(TEXT('/'));
        }
    };
}

const TCHAR* FUnrealProsperoPlatformProcess::UserTempDir()
{
    static const TemporaryDirectory directory;
    return directory.Path.c_str();
}

#if UNREALPROSPERO_TARGET_PS5
bool FUnrealProsperoPlatformProcess::ShouldSaveToUserDir()
{
    return true;
}

const TCHAR* FUnrealProsperoPlatformProcess::UserSettingsDir()
{
    return UserTempDir();
}

const TCHAR* FUnrealProsperoPlatformProcess::ApplicationSettingsDir()
{
    return UserSettingsDir();
}

FString FUnrealProsperoPlatformProcess::GetApplicationSettingsDir(const ApplicationSettingsContext&)
{
    return ApplicationSettingsDir();
}
#endif
