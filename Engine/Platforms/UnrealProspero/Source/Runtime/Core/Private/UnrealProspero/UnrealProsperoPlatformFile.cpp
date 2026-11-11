#include "UnrealProspero/UnrealProsperoPlatformFile.h"
#include <stdexcept>

IPlatformFile& IPlatformFile::GetPlatformPhysical()
{
    static FUnrealProsperoPlatformFile platform;
    return platform;
}

bool FUnrealProsperoPlatformFile::Initialize(IPlatformFile* inner, const TCHAR*)
{
    if (inner != nullptr) throw std::invalid_argument("Physical file system cannot wrap another file system");
    return true;
}

void FUnrealProsperoPlatformFile::SetLowerLevel(IPlatformFile*)
{
    throw std::logic_error("Physical file system cannot wrap another file system");
}

FString FUnrealProsperoPlatformFile::GetFilenameOnDisk(const TCHAR*)
{
    throw std::runtime_error("Canonical on-disk file paths are not implemented");
}
