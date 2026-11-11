#ifndef UNREALPROSPERO_FILE_METADATA_H
#define UNREALPROSPERO_FILE_METADATA_H

#include "GenericPlatform/GenericPlatformFile.h"
#include "SceTypes.hpp"

namespace UnrealProspero::File
{
    class Metadata final
    {
    public:
        static FFileStatData FromStat(const FileStat& status);
    };
}

#endif
