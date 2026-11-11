#ifndef UNREALPROSPERO_FILE_TIMESTAMP_H
#define UNREALPROSPERO_FILE_TIMESTAMP_H

#include "Misc/DateTime.h"
#include "SceTypes.hpp"

namespace UnrealProspero::File
{
    class Timestamp final
    {
    public:
        static FDateTime ToDateTime(const KernelTimespec& time);
        static KernelTimeval ToTimeval(FDateTime time);
    };
}

#endif
