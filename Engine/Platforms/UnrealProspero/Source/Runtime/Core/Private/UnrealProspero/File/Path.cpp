#include "UnrealProspero/File/Path.h"
#include "UnrealProspero/Text/Encoding.h"
#include <stdexcept>

namespace UnrealProspero::File
{
    std::basic_string_view<TCHAR> ViewPath(const TCHAR* path)
    {
        if (path == nullptr || *path == 0) throw std::invalid_argument("File path is empty");
        return path;
    }

    std::string EncodePath(const TCHAR* path)
    {
        return Text::EncodeUtf8(ViewPath(path));
    }
}
