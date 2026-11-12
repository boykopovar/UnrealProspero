#include "UnrealProspero/UnrealProsperoPlatformFile.h"
#include "UnrealProspero/File/DirectoryVisitor.h"
#include "UnrealProspero/File/Path.h"
#include "UnrealProspero/Kernel/Error.h"
#include "UnrealProspero/Kernel/FileSystem.h"
#include "prx/libkernel/KernelErrors.hpp"

using UnrealProspero::File::EncodePath;
using UnrealProspero::Kernel::FileSystem;

bool FUnrealProsperoPlatformFile::DirectoryExists(const TCHAR* directory)
{
    const auto status = FileSystem::Stat(EncodePath(directory));
    return status && FileSystem::IsDirectory(*status);
}

bool FUnrealProsperoPlatformFile::CreateDirectory(const TCHAR* directory)
{
    const auto path = EncodePath(directory);
    try
    {
        FileSystem::CreateDirectory(path);
    }
    catch (const UnrealProspero::Kernel::Error& error)
    {
        if (error.Code() == SCE_KERNEL_ERROR_ENOENT || error.Code() == SCE_KERNEL_ERROR_ENOTDIR) return false;
        if (error.Code() != SCE_KERNEL_ERROR_EEXIST) throw;
        const auto status = FileSystem::Stat(path);
        return status && FileSystem::IsDirectory(*status);
    }
    return true;
}

bool FUnrealProsperoPlatformFile::DeleteDirectory(const TCHAR* directory)
{
    try
    {
        FileSystem::RemoveDirectory(EncodePath(directory));
    }
    catch (const UnrealProspero::Kernel::Error& error)
    {
        if (error.Code() == SCE_KERNEL_ERROR_ENOENT || error.Code() == SCE_KERNEL_ERROR_ENOTDIR) return false;
        throw;
    }
    return true;
}

bool FUnrealProsperoPlatformFile::IterateDirectory(const TCHAR* directory, FDirectoryVisitor& visitor)
{
    return UnrealProspero::File::DirectoryVisitor::Iterate(UnrealProspero::File::ViewPath(directory), visitor);
}

bool FUnrealProsperoPlatformFile::IterateDirectoryStat(const TCHAR* directory, FDirectoryStatVisitor& visitor)
{
    return UnrealProspero::File::DirectoryVisitor::Iterate(UnrealProspero::File::ViewPath(directory), visitor);
}
