#include "UnrealProspero/UnrealProsperoPlatformProcess.h"
#include "UnrealProspero/File/TemporaryStorage.h"
#include "UnrealProspero/Text/Encoding.h"
#include <string>

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
