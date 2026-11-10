#include "AnyUnreal/Kernel/FileSystem.h"
#include "AnyUnreal/Kernel/Path.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include "prx/libkernel/File/include/File.hpp"
#include <array>

namespace AnyUnreal::Kernel
{
    namespace
    {
        void validatePermissions(std::uint16_t permissions)
        {
            if ((permissions & ~07777) != 0) throw std::invalid_argument("Invalid file permissions");
        }
    }

    std::optional<FileStat> FileSystem::Stat(std::string_view path)
    {
        FileStat status{};
        const int result = sceKernelStat(Path(path).CStr(), &status);
        if (result == SCE_KERNEL_ERROR_ENOENT || result == SCE_KERNEL_ERROR_ENOTDIR) return std::nullopt;
        CheckResult(result, "sceKernelStat");
        return status;
    }

    bool FileSystem::IsDirectory(const FileStat& status)
    {
        return (status.st_mode & 0170000) == 0040000;
    }

    bool FileSystem::IsRegularFile(const FileStat& status)
    {
        return (status.st_mode & 0170000) == 0100000;
    }

    void FileSystem::CreateDirectory(std::string_view path, std::uint16_t permissions)
    {
        validatePermissions(permissions);
        CheckResult(sceKernelMkdir(Path(path).CStr(), permissions), "sceKernelMkdir");
    }

    void FileSystem::RemoveDirectory(std::string_view path)
    {
        CheckResult(sceKernelRmdir(Path(path).CStr()), "sceKernelRmdir");
    }

    void FileSystem::RemoveFile(std::string_view path)
    {
        CheckResult(sceKernelUnlink(Path(path).CStr()), "sceKernelUnlink");
    }

    void FileSystem::RenameReplacing(std::string_view source, std::string_view destination)
    {
        const Path sourcePath(source);
        const Path destinationPath(destination);
        CheckResult(sceKernelRename(sourcePath.CStr(), destinationPath.CStr()), "sceKernelRename");
    }

    void FileSystem::SetPermissions(std::string_view path, std::uint16_t permissions)
    {
        validatePermissions(permissions);
        CheckResult(sceKernelChmod_nid_postfix(Path(path).CStr(), permissions), "sceKernelChmod");
    }

    void FileSystem::SetTimes(std::string_view path, const KernelTimeval& access, const KernelTimeval& modification)
    {
        const std::array<KernelTimeval, 2> times{access, modification};
        for (const auto& time : times)
        {
            if (time.tv_usec < 0 || time.tv_usec >= 1000000) throw std::invalid_argument("Invalid file timestamp");
        }
        CheckResult(sceKernelUtimes_nid_postfix(Path(path).CStr(), times.data()), "sceKernelUtimes");
    }
}
