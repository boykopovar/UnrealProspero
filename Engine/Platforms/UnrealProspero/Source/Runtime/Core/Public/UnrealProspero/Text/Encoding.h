#ifndef UNREALPROSPERO_TEXT_ENCODING_H
#define UNREALPROSPERO_TEXT_ENCODING_H

#include "CoreTypes.h"
#include <string>
#include <string_view>

namespace UnrealProspero::Text
{
    CORE_API std::basic_string<TCHAR> DecodeUtf8(std::string_view text);
    CORE_API std::string EncodeUtf8(std::basic_string_view<TCHAR> text);
}

#endif
