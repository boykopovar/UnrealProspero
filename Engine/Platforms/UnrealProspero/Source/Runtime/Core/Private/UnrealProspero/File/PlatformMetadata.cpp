#include "UnrealProspero/UnrealProsperoPlatformFile.h"
#include "UnrealProspero/File/Metadata.h"
#include "UnrealProspero/File/Path.h"
#include "UnrealProspero/File/Timestamp.h"
#include "UnrealProspero/Kernel/FileSystem.h"
#include <stdexcept>

using UnrealProspero::File::EncodePath;
using UnrealProspero::File::Timestamp;
using UnrealProspero::Kernel::FileSystem;

bool FUnrealProsperoPlatformFile::FileExists(const TCHAR* filename)
{
    const auto status = FileSystem::Stat(EncodePath(filename));
    return status && FileSystem::IsRegularFile(*status);
}

int64 FUnrealProsperoPlatformFile::FileSize(const TCHAR* filename)
{
    const auto status = FileSystem::Stat(EncodePath(filename));
    if (!status || !FileSystem::IsRegularFile(*status)) return -1;
    if (status->st_size < 0) throw std::runtime_error("Negative file size");
    return status->st_size;
}

bool FUnrealProsperoPlatformFile::IsReadOnly(const TCHAR* filename)
{
    const auto status = FileSystem::Stat(EncodePath(filename));
    return status && (status->st_mode & 0200) == 0;
}

bool FUnrealProsperoPlatformFile::SetReadOnly(const TCHAR* filename, bool readOnly)
{
    const auto path = EncodePath(filename);
    const auto status = FileSystem::Stat(path);
    if (!status) return false;
    auto permissions = static_cast<std::uint16_t>(status->st_mode & 07777);
    if (readOnly) permissions &= ~0200;
    else permissions |= 0200;
    FileSystem::SetPermissions(path, permissions);
    return true;
}

FDateTime FUnrealProsperoPlatformFile::GetTimeStamp(const TCHAR* filename)
{
    const auto status = FileSystem::Stat(EncodePath(filename));
    return status ? Timestamp::ToDateTime(status->st_mtim) : FDateTime::MinValue();
}

FDateTime FUnrealProsperoPlatformFile::GetAccessTimeStamp(const TCHAR* filename)
{
    const auto status = FileSystem::Stat(EncodePath(filename));
    return status ? Timestamp::ToDateTime(status->st_atim) : FDateTime::MinValue();
}

void FUnrealProsperoPlatformFile::SetTimeStamp(const TCHAR* filename, FDateTime time)
{
    const auto path = EncodePath(filename);
    const auto status = FileSystem::Stat(path);
    if (!status) throw std::runtime_error("Cannot set timestamp of a missing file");
    FileSystem::SetTimes(path, Timestamp::ToTimeval(Timestamp::ToDateTime(status->st_atim)), Timestamp::ToTimeval(time));
}

FFileStatData FUnrealProsperoPlatformFile::GetStatData(const TCHAR* filename)
{
    const auto status = FileSystem::Stat(EncodePath(filename));
    return status ? UnrealProspero::File::Metadata::FromStat(*status) : FFileStatData();
}
