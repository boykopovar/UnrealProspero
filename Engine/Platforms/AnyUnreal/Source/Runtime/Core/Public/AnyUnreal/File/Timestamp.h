#ifndef ANYUNREAL_FILE_TIMESTAMP_H
#define ANYUNREAL_FILE_TIMESTAMP_H

#include "Misc/DateTime.h"
#include "SceTypes.hpp"

namespace AnyUnreal::File
{
    class Timestamp final
    {
    public:
        static FDateTime ToDateTime(const KernelTimespec& time);
        static KernelTimeval ToTimeval(FDateTime time);
    };
}

#endif
