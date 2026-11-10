#include "HAL/PlatformString.h"
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
    return true;
}
