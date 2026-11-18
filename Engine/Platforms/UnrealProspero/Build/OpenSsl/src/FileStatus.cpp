#include "prx/libkernel/include/Api.hpp"
#include <sys/stat.h>

namespace
{
    int CopyStatus(int result, const FileStat& source, struct stat* destination)
    {
        if (result != 0) return result;
        *destination = {};
        destination->st_dev = source.st_dev;
        destination->st_ino = source.st_ino;
        destination->st_mode = source.st_mode;
        destination->st_nlink = source.st_nlink;
        destination->st_uid = source.st_uid;
        destination->st_gid = source.st_gid;
        destination->st_rdev = source.st_rdev;
        destination->st_atim = {source.st_atim.tv_sec, source.st_atim.tv_nsec};
        destination->st_mtim = {source.st_mtim.tv_sec, source.st_mtim.tv_nsec};
        destination->st_ctim = {source.st_ctim.tv_sec, source.st_ctim.tv_nsec};
        destination->st_size = source.st_size;
        destination->st_blocks = source.st_blocks;
        destination->st_blksize = source.st_blksize;
        return 0;
    }
}

extern "C" int UnrealProsperoOpenSslStat(const char* path, struct stat* status)
{
    FileStat guest{};
    const int result = stat_nid_postfix(path, &guest);
    return CopyStatus(result, guest, status);
}

extern "C" int UnrealProsperoOpenSslLstat(const char* path, struct stat* status)
{
    FileStat guest{};
    const int result = lstat_nid_postfix(path, &guest);
    return CopyStatus(result, guest, status);
}
