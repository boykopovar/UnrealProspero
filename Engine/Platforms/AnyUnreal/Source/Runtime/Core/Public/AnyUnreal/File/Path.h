#ifndef ANYUNREAL_FILE_PATH_H
#define ANYUNREAL_FILE_PATH_H

#include "CoreTypes.h"
#include <string>
#include <string_view>

namespace AnyUnreal::File
{
    std::basic_string_view<TCHAR> ViewPath(const TCHAR* path);
    std::string EncodePath(const TCHAR* path);
}

#endif
