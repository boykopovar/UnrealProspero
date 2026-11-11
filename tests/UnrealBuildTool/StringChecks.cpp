#include "HAL/PlatformString.h"
#include "UnrealProspero/Text/Encoding.h"
#include <stdexcept>
#include <type_traits>

namespace
{
    template<typename TChar>
    bool checkTokens()
    {
        TChar first[]{',', 'a', ',', ',', 'b', ';', 'c', 0};
        TChar second[]{'x', ',', 'y', 0};
        const TChar comma[]{',', 0};
        const TChar semicolon[]{';', 0};
        const TChar empty[]{0};
        TChar* firstContext = nullptr;
        TChar* secondContext = nullptr;
        if (FPlatformString::Strtok(first, comma, &firstContext) != first + 1 || first[2] != 0) return false;
        if (FPlatformString::Strtok(second, comma, &secondContext) != second || second[1] != 0) return false;
        if (FPlatformString::Strtok(nullptr, semicolon, &firstContext) != first + 3 || first[5] != 0) return false;
        if (FPlatformString::Strtok(nullptr, empty, &secondContext) != second + 2) return false;
        if (FPlatformString::Strtok(nullptr, comma, &firstContext) != first + 6) return false;
        if (FPlatformString::Strtok(nullptr, comma, &firstContext) != nullptr) return false;
        if (FPlatformString::Strtok(nullptr, comma, &secondContext) != nullptr) return false;
        TChar delimitersOnly[]{',', ',', 0};
        if (FPlatformString::Strtok(delimitersOnly, comma, &firstContext) != nullptr) return false;
        try
        {
            FPlatformString::Strtok(first, comma, static_cast<TChar**>(nullptr));
            return false;
        }
        catch (const std::invalid_argument&)
        {
        }
        return true;
    }
}

bool CheckStrings()
{
    static_assert(std::is_same_v<SIZE_T, std::size_t>);
    static_assert(std::is_same_v<SSIZE_T, std::ptrdiff_t>);
    static_assert(std::is_same_v<PTRINT, long long>);
    static_assert(std::is_same_v<UPTRINT, unsigned long long>);
    if (!checkTokens<ANSICHAR>() || !checkTokens<WIDECHAR>() || !checkTokens<UTF8CHAR>()) return false;
    const TCHAR text[]{'a', 0x0411, 'c', 0};
    if (FPlatformString::Strlen(text) != 3 || FPlatformString::Strnlen(text, 2) != 2) return false;
    if (FPlatformString::Strnlen(text, 0) != 0 || FPlatformString::Strnlen(text, 8) != 3) return false;
    if (FPlatformString::Strchr(text, static_cast<TCHAR>(0x0411)) != text + 1) return false;
    if (FPlatformString::Strcmp(text, text) != 0) return false;
    if (FPlatformString::Strlen("abc") != 3) return false;
    using UnrealProspero::Text::DecodeUtf8;
    using UnrealProspero::Text::EncodeUtf8;
    const std::string encoded = "A\xc2\x80\xdf\xbf\xe0\xa0\x80\xef\xbf\xbf\xf0\x90\x80\x80\xf4\x8f\xbf\xbf";
    const std::basic_string<TCHAR> decoded{'A', 0x80, 0x7ff, 0x800, 0xffff, 0xd800, 0xdc00, 0xdbff, 0xdfff};
    if (DecodeUtf8(encoded) != decoded || EncodeUtf8(decoded) != encoded) return false;
    if (!DecodeUtf8("").empty()) return false;
    if (DecodeUtf8(std::string_view("a\0b", 3)).size() != 3) return false;
    for (const char* invalid : {"\x80", "\xc0\xaf", "\xc1\xbf", "\xc2", "\xc2x", "\xe0\x80\xaf", "\xed\xa0\x80", "\xf0\x80\x80\xaf", "\xf4\x90\x80\x80", "\xf5\x80\x80\x80", "\xff"})
    {
        try
        {
            DecodeUtf8(invalid);
            return false;
        }
        catch (const std::invalid_argument&)
        {
        }
    }
    return true;
}
