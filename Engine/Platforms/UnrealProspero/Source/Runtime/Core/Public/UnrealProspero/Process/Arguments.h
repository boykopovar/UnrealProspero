#ifndef UNREALPROSPERO_PROCESS_ARGUMENTS_H
#define UNREALPROSPERO_PROCESS_ARGUMENTS_H

#include "CoreTypes.h"
#include <string>
#include <vector>

namespace UnrealProspero::Process
{
    class Arguments final
    {
    public:
        static std::vector<std::basic_string<TCHAR>> Read();
        static std::vector<std::basic_string<TCHAR>> Decode(int count, const char* const* arguments);
    };
}

#endif
