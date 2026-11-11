#include "UnrealProspero/Kernel/File.h"
#include "UnrealProspero/Kernel/FileSystem.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include <array>
#include <string>

namespace
{
    using UnrealProspero::Kernel::FileSystem;

    class Directory
    {
    public:
        explicit Directory(std::string path) : path(std::move(path))
        {
            FileSystem::CreateDirectory(this->path);
        }

        ~Directory() noexcept(false)
        {
            for (const char* name : {"first", "second"})
            {
                const auto child = path + "/" + name;
                if (FileSystem::Stat(child))
                {
                    FileSystem::SetPermissions(child, 0666);
                    FileSystem::RemoveFile(child);
                }
            }
            FileSystem::RemoveDirectory(path);
        }

    private:
        std::string path;
    };
}

bool CheckFileSystem()
{
    using UnrealProspero::Kernel::File;
    using UnrealProspero::Kernel::FileAccess;
    using UnrealProspero::Kernel::FileCreation;
    using UnrealProspero::Kernel::Error;
    const auto root = "UnrealProspero-directory-" + std::to_string(getpid_nid_postfix());
    if (FileSystem::Stat(root)) return false;
    {
        Directory directory(root);
        const auto status = FileSystem::Stat(root);
        if (!status || !FileSystem::IsDirectory(*status) || FileSystem::IsRegularFile(*status)) return false;
        try
        {
            FileSystem::CreateDirectory(root);
            return false;
        }
        catch (const Error& error)
        {
            if (error.Code() != SCE_KERNEL_ERROR_EEXIST) return false;
        }
        const auto first = root + "/first";
        const auto second = root + "/second";
        {
            File file(first, FileAccess::Write, FileCreation::CreateNew);
            const std::array<std::byte, 3> data{std::byte{1}, std::byte{2}, std::byte{3}};
            file.Write(data);
        }
        const auto fileStatus = FileSystem::Stat(first);
        if (!fileStatus || !FileSystem::IsRegularFile(*fileStatus) || FileSystem::IsDirectory(*fileStatus) || fileStatus->st_size != 3) return false;
        if (FileSystem::Stat(first + "/child")) return false;
        {
            File file(second, FileAccess::Write, FileCreation::CreateNew);
        }
        if (FileSystem::MoveNoReplace(first, second)) return false;
        const auto untouchedSource = FileSystem::Stat(first);
        const auto untouchedDestination = FileSystem::Stat(second);
        if (!untouchedSource || untouchedSource->st_size != 3 || !untouchedDestination || untouchedDestination->st_size != 0) return false;
        FileSystem::RemoveFile(second);
        if (!FileSystem::MoveNoReplace(first, second)) return false;
        const auto moved = FileSystem::Stat(second);
        if (!moved || moved->st_ino != untouchedSource->st_ino || moved->st_dev != untouchedSource->st_dev) return false;
        if (FileSystem::Stat(first)) return false;
        try
        {
            FileSystem::MoveNoReplace(first, first + "-absent");
            return false;
        }
        catch (const Error& error)
        {
            if (error.Code() != SCE_KERNEL_ERROR_ENOENT) return false;
        }
        FileSystem::RenameReplacing(second, first);
        {
            File file(second, FileAccess::Write, FileCreation::CreateNew);
        }
        FileSystem::RenameReplacing(first, second);
        const auto renamed = FileSystem::Stat(second);
        if (FileSystem::Stat(first) || !renamed || renamed->st_size != 3) return false;
        FileSystem::SetTimes(second, KernelTimeval{1700000000, 123000}, KernelTimeval{1700000123, 456000});
        const auto timed = FileSystem::Stat(second);
        if (!timed || timed->st_atim.tv_sec != 1700000000 || timed->st_mtim.tv_sec != 1700000123) return false;
        if (timed->st_atim.tv_nsec != 123000000 || timed->st_mtim.tv_nsec != 456000000) return false;
        FileSystem::SetPermissions(second, 0444);
        const auto readOnly = FileSystem::Stat(second);
        if (!readOnly || (readOnly->st_mode & 0222) != 0) return false;
        FileSystem::SetPermissions(second, 0666);
        const auto writable = FileSystem::Stat(second);
        if (!writable || (writable->st_mode & 0200) == 0) return false;
        try
        {
            FileSystem::SetTimes(second, KernelTimeval{0, -1}, KernelTimeval{0, 0});
            return false;
        }
        catch (const std::invalid_argument&)
        {
        }
        try
        {
            FileSystem::SetPermissions(second, 0177777);
            return false;
        }
        catch (const std::invalid_argument&)
        {
        }
    }
    return !FileSystem::Stat(root);
}
