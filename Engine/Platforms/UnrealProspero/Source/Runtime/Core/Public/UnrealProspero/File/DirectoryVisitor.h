#ifndef UNREALPROSPERO_FILE_DIRECTORY_VISITOR_H
#define UNREALPROSPERO_FILE_DIRECTORY_VISITOR_H

#include "GenericPlatform/GenericPlatformFile.h"
#include <string_view>

namespace UnrealProspero::File
{
    class DirectoryVisitor final
    {
    public:
        static bool Iterate(std::basic_string_view<TCHAR> path, IPlatformFile::FDirectoryVisitor& visitor);
        static bool Iterate(std::basic_string_view<TCHAR> path, IPlatformFile::FDirectoryStatVisitor& visitor);
    };
}

#endif
