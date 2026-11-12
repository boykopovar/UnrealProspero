#include "UnrealProspero/UnrealProsperoPlatformFile.h"
#include "UnrealProspero/File/FileHandle.h"
#include "UnrealProspero/File/Path.h"
#include "UnrealProspero/Kernel/FileSystem.h"
#include "UnrealProspero/Kernel/Error.h"
#include "prx/libkernel/KernelErrors.hpp"
#include <memory>

using UnrealProspero::File::EncodePath;
using UnrealProspero::Kernel::FileAccess;
using UnrealProspero::Kernel::FileCreation;
using UnrealProspero::Kernel::FileSystem;

IFileHandle* FUnrealProsperoPlatformFile::OpenRead(const TCHAR* filename, bool)
{
    try
    {
        return new FUnrealProsperoFileHandle(EncodePath(filename), FileAccess::Read);
    }
    catch (const UnrealProspero::Kernel::Error& error)
    {
        if (error.Code() == SCE_KERNEL_ERROR_ENOENT || error.Code() == SCE_KERNEL_ERROR_ENOTDIR) return nullptr;
        throw;
    }
}

IFileHandle* FUnrealProsperoPlatformFile::OpenWrite(const TCHAR* filename, bool append, bool allowRead)
{
    try
    {
        const auto access = allowRead ? FileAccess::ReadWrite : FileAccess::Write;
        const auto creation = append ? FileCreation::OpenOrCreate : FileCreation::CreateOrTruncate;
        auto handle = std::make_unique<FUnrealProsperoFileHandle>(EncodePath(filename), access, creation, append);
        if (append) handle->SeekFromEnd();
        return handle.release();
    }
    catch (const UnrealProspero::Kernel::Error& error)
    {
        if (error.Code() == SCE_KERNEL_ERROR_ENOENT || error.Code() == SCE_KERNEL_ERROR_ENOTDIR) return nullptr;
        throw;
    }
}

bool FUnrealProsperoPlatformFile::DeleteFile(const TCHAR* filename)
{
    const auto path = EncodePath(filename);
    const auto status = FileSystem::Stat(path);
    if (!status || !FileSystem::IsRegularFile(*status) || (status->st_mode & 0200) == 0) return false;
    FileSystem::RemoveFile(path);
    return true;
}

bool FUnrealProsperoPlatformFile::MoveFile(const TCHAR* destination, const TCHAR* source)
{
    return FileSystem::MoveNoReplace(EncodePath(source), EncodePath(destination));
}
