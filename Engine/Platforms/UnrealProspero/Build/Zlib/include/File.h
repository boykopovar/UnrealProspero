#ifndef UNREALPROSPERO_ZLIB_FILE_H
#define UNREALPROSPERO_ZLIB_FILE_H

#include <cstddef>
#include <cstdint>

#ifdef _WIN32
using UnrealProsperoZlibTransferSize = unsigned;
using UnrealProsperoZlibTransferResult = int;
#else
using UnrealProsperoZlibTransferSize = std::size_t;
using UnrealProsperoZlibTransferResult = std::int64_t;
#endif

extern "C"
{
    int UnrealProsperoZlibOpen(const char* path, int flags, ...);
    int UnrealProsperoZlibClose(int descriptor);
    UnrealProsperoZlibTransferResult UnrealProsperoZlibRead(int descriptor, void* buffer, UnrealProsperoZlibTransferSize count);
    UnrealProsperoZlibTransferResult UnrealProsperoZlibWrite(int descriptor, const void* buffer, UnrealProsperoZlibTransferSize count);
    std::int64_t UnrealProsperoZlibSeek(int descriptor, std::int64_t offset, int origin);
#ifdef _WIN32
    int UnrealProsperoZlibWideOpen(const wchar_t* path, int flags, ...);
#endif
}

#endif
