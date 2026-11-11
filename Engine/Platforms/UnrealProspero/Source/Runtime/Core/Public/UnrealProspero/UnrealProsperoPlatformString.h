#ifndef UNREALPROSPERO_PLATFORM_STRING_H
#define UNREALPROSPERO_PLATFORM_STRING_H

#include "GenericPlatform/GenericWidePlatformString.h"
#include <stdexcept>
#include <type_traits>

struct FUnrealProsperoPlatformString : FGenericWidePlatformString
{
    template<typename TChar>
    static TChar* Strtok(std::type_identity_t<TChar>* text, const TChar* delimiters, TChar** context)
    {
        static_assert(std::is_same_v<TChar, ANSICHAR> || std::is_same_v<TChar, WIDECHAR> || std::is_same_v<TChar, UTF8CHAR>);
        if (delimiters == nullptr || context == nullptr) throw std::invalid_argument("Invalid string token context or delimiters");
        TChar* cursor = text == nullptr ? *context : text;
        if (cursor == nullptr) return nullptr;
        auto isDelimiter = [delimiters](TChar character)
        {
            for (const TChar* delimiter = delimiters; *delimiter != 0; ++delimiter)
            {
                if (*delimiter == character) return true;
            }
            return false;
        };
        while (*cursor != 0 && isDelimiter(*cursor)) ++cursor;
        if (*cursor == 0)
        {
            *context = cursor;
            return nullptr;
        }
        TChar* token = cursor;
        while (*cursor != 0 && !isDelimiter(*cursor)) ++cursor;
        if (*cursor != 0) *cursor++ = 0;
        *context = cursor;
        return token;
    }
};

using FPlatformString = FUnrealProsperoPlatformString;

#define SIZE_T_FMT "zu"
#define SIZE_T_x_FMT "zx"
#define SIZE_T_X_FMT "zX"
#define SSIZE_T_FMT "td"
#define SSIZE_T_x_FMT "tx"
#define SSIZE_T_X_FMT "tX"
#define UPTRINT_FMT "llu"
#define UPTRINT_x_FMT "llx"
#define UPTRINT_X_FMT "llX"
#define PTRINT_FMT "lld"
#define PTRINT_x_FMT "llx"
#define PTRINT_X_FMT "llX"
#define INT64_FMT "lld"
#define INT64_x_FMT "llx"
#define INT64_X_FMT "llX"
#define UINT64_FMT "llu"
#define UINT64_x_FMT "llx"
#define UINT64_X_FMT "llX"

#endif
