#include "AnyUnreal/AnyUnrealPlatformFile.h"
#include "AnyUnreal/File/DirectoryVisitor.h"
#include "AnyUnreal/File/Path.h"
#include "AnyUnreal/Kernel/Error.h"
#include "AnyUnreal/Kernel/FileSystem.h"
#include "prx/libkernel/KernelErrors.hpp"

using AnyUnreal::File::EncodePath;
using AnyUnreal::Kernel::FileSystem;

bool FAnyUnrealPlatformFile::DirectoryExists(const TCHAR* directory)
{
    const auto status = FileSystem::Stat(EncodePath(directory));
    return status && FileSystem::IsDirectory(*status);
}

bool FAnyUnrealPlatformFile::CreateDirectory(const TCHAR* directory)
{
    const auto path = EncodePath(directory);
    try
    {
        FileSystem::CreateDirectory(path);
    }
    catch (const AnyUnreal::Kernel::Error& error)
    {
        if (error.Code() != SCE_KERNEL_ERROR_EEXIST) throw;
        const auto status = FileSystem::Stat(path);
        return status && FileSystem::IsDirectory(*status);
    }
    return true;
}

bool FAnyUnrealPlatformFile::DeleteDirectory(const TCHAR* directory)
{
    try
    {
        FileSystem::RemoveDirectory(EncodePath(directory));
    }
    catch (const AnyUnreal::Kernel::Error& error)
    {
        if (error.Code() == SCE_KERNEL_ERROR_ENOENT || error.Code() == SCE_KERNEL_ERROR_ENOTDIR) return false;
        throw;
    }
    return true;
}

bool FAnyUnrealPlatformFile::IterateDirectory(const TCHAR* directory, FDirectoryVisitor& visitor)
{
    return AnyUnreal::File::DirectoryVisitor::Iterate(AnyUnreal::File::ViewPath(directory), visitor);
}

bool FAnyUnrealPlatformFile::IterateDirectoryStat(const TCHAR* directory, FDirectoryStatVisitor& visitor)
{
    return AnyUnreal::File::DirectoryVisitor::Iterate(AnyUnreal::File::ViewPath(directory), visitor);
}
