#ifndef ANYUNREAL_FILE_METADATA_H
#define ANYUNREAL_FILE_METADATA_H

#include "GenericPlatform/GenericPlatformFile.h"
#include "SceTypes.hpp"

namespace AnyUnreal::File
{
    class Metadata final
    {
    public:
        static FFileStatData FromStat(const FileStat& status);
    };
}

#endif
