#ifndef ANYUNREAL_TEXT_ENCODING_H
#define ANYUNREAL_TEXT_ENCODING_H

#include "CoreTypes.h"
#include <string>
#include <string_view>

namespace AnyUnreal::Text
{
    CORE_API std::string EncodeUtf8(std::basic_string_view<TCHAR> text);
}

#endif
