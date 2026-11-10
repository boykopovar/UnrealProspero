#include "AnyUnreal/File/Metadata.h"
#include "AnyUnreal/File/Timestamp.h"
#include "AnyUnreal/Kernel/FileSystem.h"
#include <stdexcept>

namespace AnyUnreal::File
{
    FFileStatData Metadata::FromStat(const FileStat& status)
    {
        const bool directory = Kernel::FileSystem::IsDirectory(status);
        if (!directory && status.st_size < 0) throw std::invalid_argument("Negative file size in stat data");
        return FFileStatData(Timestamp::ToDateTime(status.st_birthtim), Timestamp::ToDateTime(status.st_atim), Timestamp::ToDateTime(status.st_mtim), directory ? -1 : status.st_size, directory, (status.st_mode & 0200) == 0);
    }
}
