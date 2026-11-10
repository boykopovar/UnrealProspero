#include "AnyUnreal/AnyUnrealPlatformFile.h"
#include <stdexcept>

IPlatformFile& IPlatformFile::GetPlatformPhysical()
{
    static FAnyUnrealPlatformFile platform;
    return platform;
}

bool FAnyUnrealPlatformFile::Initialize(IPlatformFile* inner, const TCHAR*)
{
    if (inner != nullptr) throw std::invalid_argument("Physical file system cannot wrap another file system");
    return true;
}

void FAnyUnrealPlatformFile::SetLowerLevel(IPlatformFile*)
{
    throw std::logic_error("Physical file system cannot wrap another file system");
}

bool FAnyUnrealPlatformFile::MoveFile(const TCHAR*, const TCHAR*)
{
    throw std::runtime_error("Non-replacing file moves are not implemented");
}

FString FAnyUnrealPlatformFile::GetFilenameOnDisk(const TCHAR*)
{
    throw std::runtime_error("Canonical on-disk file paths are not implemented");
}
