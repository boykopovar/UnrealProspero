#ifndef ANYUNREAL_PROCESS_ARGUMENTS_H
#define ANYUNREAL_PROCESS_ARGUMENTS_H

#include "CoreTypes.h"
#include <string>
#include <vector>

namespace AnyUnreal::Process
{
    class Arguments final
    {
    public:
        static std::vector<std::basic_string<TCHAR>> Read();
        static std::vector<std::basic_string<TCHAR>> Decode(int count, const char* const* arguments);
    };
}

#endif
