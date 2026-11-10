#ifndef ANYUNREAL_KERNEL_FILE_SYSTEM_H
#define ANYUNREAL_KERNEL_FILE_SYSTEM_H

#include "SceTypes.hpp"
#include <optional>
#include <string_view>

namespace AnyUnreal::Kernel
{
    class FileSystem final
    {
    public:
        static std::optional<FileStat> Stat(std::string_view path);
        static bool IsDirectory(const FileStat& status);
        static bool IsRegularFile(const FileStat& status);
        static void CreateDirectory(std::string_view path, std::uint16_t permissions = 0777);
        static void RemoveDirectory(std::string_view path);
        static void RemoveFile(std::string_view path);
        static void RenameReplacing(std::string_view source, std::string_view destination);
        static void SetPermissions(std::string_view path, std::uint16_t permissions);
        static void SetTimes(std::string_view path, const KernelTimeval& access, const KernelTimeval& modification);
    };
}

#endif
