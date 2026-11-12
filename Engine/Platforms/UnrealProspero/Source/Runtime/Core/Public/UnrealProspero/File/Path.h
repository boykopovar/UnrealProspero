#ifndef UNREALPROSPERO_FILE_PATH_H
#define UNREALPROSPERO_FILE_PATH_H

#include "CoreTypes.h"
#include <string>
#include <string_view>

namespace UnrealProspero::File
{
    std::basic_string_view<TCHAR> ViewPath(const TCHAR* path);
    std::string ResolveStagedPath(std::string_view root, std::string_view path);
    std::string EncodePath(std::basic_string_view<TCHAR> path);
    std::string EncodePath(const TCHAR* path);
}

#endif
