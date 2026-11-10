#ifndef ANYUNREAL_FILE_DIRECTORY_VISITOR_H
#define ANYUNREAL_FILE_DIRECTORY_VISITOR_H

#include "GenericPlatform/GenericPlatformFile.h"
#include <string_view>

namespace AnyUnreal::File
{
    class DirectoryVisitor final
    {
    public:
        static bool Iterate(std::basic_string_view<TCHAR> path, IPlatformFile::FDirectoryVisitor& visitor);
        static bool Iterate(std::basic_string_view<TCHAR> path, IPlatformFile::FDirectoryStatVisitor& visitor);
    };
}

#endif
