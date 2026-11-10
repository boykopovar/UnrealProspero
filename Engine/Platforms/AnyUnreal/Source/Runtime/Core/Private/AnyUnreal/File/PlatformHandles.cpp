#include "AnyUnreal/AnyUnrealPlatformFile.h"
#include "AnyUnreal/File/FileHandle.h"
#include "AnyUnreal/File/Path.h"
#include "AnyUnreal/Kernel/FileSystem.h"
#include <memory>

using AnyUnreal::File::EncodePath;
using AnyUnreal::Kernel::FileAccess;
using AnyUnreal::Kernel::FileCreation;
using AnyUnreal::Kernel::FileSystem;

IFileHandle* FAnyUnrealPlatformFile::OpenRead(const TCHAR* filename, bool)
{
    return new FAnyUnrealFileHandle(EncodePath(filename), FileAccess::Read);
}

IFileHandle* FAnyUnrealPlatformFile::OpenWrite(const TCHAR* filename, bool append, bool allowRead)
{
    const auto access = allowRead ? FileAccess::ReadWrite : FileAccess::Write;
    const auto creation = append ? FileCreation::OpenOrCreate : FileCreation::CreateOrTruncate;
    auto handle = std::make_unique<FAnyUnrealFileHandle>(EncodePath(filename), access, creation, append);
    if (append) handle->SeekFromEnd();
    return handle.release();
}

bool FAnyUnrealPlatformFile::DeleteFile(const TCHAR* filename)
{
    const auto path = EncodePath(filename);
    const auto status = FileSystem::Stat(path);
    if (!status || !FileSystem::IsRegularFile(*status) || (status->st_mode & 0200) == 0) return false;
    FileSystem::RemoveFile(path);
    return true;
}

bool FAnyUnrealPlatformFile::MoveFile(const TCHAR* destination, const TCHAR* source)
{
    return FileSystem::MoveNoReplace(EncodePath(source), EncodePath(destination));
}
