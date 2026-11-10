#include "AnyUnreal/AnyUnrealPlatformFile.h"
#include "AnyUnreal/File/Metadata.h"
#include "AnyUnreal/File/Path.h"
#include "AnyUnreal/File/Timestamp.h"
#include "AnyUnreal/Kernel/FileSystem.h"
#include <stdexcept>

using AnyUnreal::File::EncodePath;
using AnyUnreal::File::Timestamp;
using AnyUnreal::Kernel::FileSystem;

bool FAnyUnrealPlatformFile::FileExists(const TCHAR* filename)
{
    const auto status = FileSystem::Stat(EncodePath(filename));
    return status && FileSystem::IsRegularFile(*status);
}

int64 FAnyUnrealPlatformFile::FileSize(const TCHAR* filename)
{
    const auto status = FileSystem::Stat(EncodePath(filename));
    if (!status || !FileSystem::IsRegularFile(*status)) return -1;
    if (status->st_size < 0) throw std::runtime_error("Negative file size");
    return status->st_size;
}

bool FAnyUnrealPlatformFile::IsReadOnly(const TCHAR* filename)
{
    const auto status = FileSystem::Stat(EncodePath(filename));
    return status && (status->st_mode & 0200) == 0;
}

bool FAnyUnrealPlatformFile::SetReadOnly(const TCHAR* filename, bool readOnly)
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

FDateTime FAnyUnrealPlatformFile::GetTimeStamp(const TCHAR* filename)
{
    const auto status = FileSystem::Stat(EncodePath(filename));
    return status ? Timestamp::ToDateTime(status->st_mtim) : FDateTime::MinValue();
}

FDateTime FAnyUnrealPlatformFile::GetAccessTimeStamp(const TCHAR* filename)
{
    const auto status = FileSystem::Stat(EncodePath(filename));
    return status ? Timestamp::ToDateTime(status->st_atim) : FDateTime::MinValue();
}

void FAnyUnrealPlatformFile::SetTimeStamp(const TCHAR* filename, FDateTime time)
{
    const auto path = EncodePath(filename);
    const auto status = FileSystem::Stat(path);
    if (!status) throw std::runtime_error("Cannot set timestamp of a missing file");
    FileSystem::SetTimes(path, Timestamp::ToTimeval(Timestamp::ToDateTime(status->st_atim)), Timestamp::ToTimeval(time));
}

FFileStatData FAnyUnrealPlatformFile::GetStatData(const TCHAR* filename)
{
    const auto status = FileSystem::Stat(EncodePath(filename));
    return status ? AnyUnreal::File::Metadata::FromStat(*status) : FFileStatData();
}
