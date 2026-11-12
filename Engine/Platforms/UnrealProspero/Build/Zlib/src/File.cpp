#include "File.h"
#include "prx/libkernel/include/Api.hpp"
#include <cstdarg>
#include <fcntl.h>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>

namespace
{
    int openFlags(int flags)
    {
        int result = flags & (O_WRONLY | O_RDWR);
        if (result == (O_WRONLY | O_RDWR)) throw std::invalid_argument("Invalid zlib file access mode");
        int known = O_WRONLY | O_RDWR | O_CREAT | O_EXCL | O_TRUNC | O_APPEND;
        if (flags & O_CREAT) result |= 0x200;
        if (flags & O_EXCL) result |= 0x800;
        if (flags & O_TRUNC) result |= 0x400;
        if (flags & O_APPEND) result |= 0x8;
#ifdef O_BINARY
        known |= O_BINARY;
#endif
#ifdef O_LARGEFILE
        known |= O_LARGEFILE;
#endif
#ifdef O_CLOEXEC
        known |= O_CLOEXEC;
        if (flags & O_CLOEXEC) result |= 0x100000;
#endif
        if (flags & ~known) throw std::invalid_argument("Unsupported zlib file flags");
        return result;
    }

    void validateCount(UnrealProsperoZlibTransferSize count)
    {
        if (count > static_cast<UnrealProsperoZlibTransferSize>(std::numeric_limits<UnrealProsperoZlibTransferResult>::max())) throw std::invalid_argument("zlib file transfer exceeds its return type");
    }
}

extern "C" int UnrealProsperoZlibOpen(const char* path, int flags, ...)
{
    int mode = 0;
    if (flags & O_CREAT)
    {
        va_list arguments;
        va_start(arguments, flags);
        mode = va_arg(arguments, int);
        va_end(arguments);
    }
    return open_nid_postfix(path, openFlags(flags), mode);
}

extern "C" int UnrealProsperoZlibClose(int descriptor)
{
    return close_nid_postfix(descriptor);
}

extern "C" UnrealProsperoZlibTransferResult UnrealProsperoZlibRead(int descriptor, void* buffer, UnrealProsperoZlibTransferSize count)
{
    validateCount(count);
    return static_cast<UnrealProsperoZlibTransferResult>(read_nid_postfix(descriptor, buffer, count));
}

extern "C" UnrealProsperoZlibTransferResult UnrealProsperoZlibWrite(int descriptor, const void* buffer, UnrealProsperoZlibTransferSize count)
{
    validateCount(count);
    return static_cast<UnrealProsperoZlibTransferResult>(write_nid_postfix(descriptor, static_cast<const char*>(buffer), count));
}

extern "C" std::int64_t UnrealProsperoZlibSeek(int descriptor, std::int64_t offset, int origin)
{
    return lseek_nid_postfix(descriptor, offset, origin);
}

#ifdef _WIN32
extern "C" int UnrealProsperoZlibWideOpen(const wchar_t* path, int flags, ...)
{
    if (!path) throw std::invalid_argument("Null zlib file path");
    int mode = 0;
    if (flags & O_CREAT)
    {
        va_list arguments;
        va_start(arguments, flags);
        mode = va_arg(arguments, int);
        va_end(arguments);
    }
    const auto utf8 = std::filesystem::path(path).u8string();
    const std::string encoded(utf8.begin(), utf8.end());
    return open_nid_postfix(encoded.c_str(), openFlags(flags), mode);
}
#endif
