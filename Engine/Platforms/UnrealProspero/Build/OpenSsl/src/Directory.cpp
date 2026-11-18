#include "prx/libc/include/Api.hpp"
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <new>

namespace
{
    struct Directory
    {
        void* handle;
        dirent entry;
    };
}

extern "C" void* UnrealProsperoOpenSslOpenDirectory(const char* path)
{
    auto* directory = new (std::nothrow) Directory{};
    if (!directory)
    {
        errno = ENOMEM;
        return nullptr;
    }
    directory->handle = opendir_nid_postfix(path);
    if (directory->handle) return directory;
    const int error = errno;
    delete directory;
    errno = error;
    return nullptr;
}

extern "C" dirent* UnrealProsperoOpenSslReadDirectory(void* handle)
{
    auto& directory = *static_cast<Directory*>(handle);
    const auto* guest = readdir_nid_postfix(directory.handle);
    if (!guest) return nullptr;
    directory.entry = {};
    directory.entry.d_ino = guest->fileNumber;
    directory.entry.d_reclen = sizeof(dirent);
    directory.entry.d_type = guest->type;
    std::memcpy(directory.entry.d_name, guest->name, guest->nameLength);
    return &directory.entry;
}

extern "C" int UnrealProsperoOpenSslCloseDirectory(void* handle)
{
    auto* directory = static_cast<Directory*>(handle);
    const int result = closedir_nid_postfix(directory->handle);
    const int error = errno;
    delete directory;
    errno = error;
    return result;
}
