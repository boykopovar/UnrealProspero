#include "UnrealProspero/Text/Encoding.h"
#include <stdexcept>

namespace UnrealProspero::Text
{
    std::basic_string<TCHAR> DecodeUtf8(std::string_view text)
    {
        static_assert(sizeof(TCHAR) == 2);
        std::basic_string<TCHAR> result;
        result.reserve(text.size());
        for (std::size_t index = 0; index < text.size();)
        {
            const auto first = static_cast<unsigned char>(text[index++]);
            uint32 value;
            uint32 minimum;
            unsigned count;
            if (first < 0x80) { value = first; minimum = 0; count = 0; }
            else if (first >= 0xc2 && first <= 0xdf) { value = first & 0x1f; minimum = 0x80; count = 1; }
            else if (first >= 0xe0 && first <= 0xef) { value = first & 0x0f; minimum = 0x800; count = 2; }
            else if (first >= 0xf0 && first <= 0xf4) { value = first & 0x07; minimum = 0x10000; count = 3; }
            else throw std::invalid_argument("Invalid UTF-8 leading byte");
            if (text.size() - index < count) throw std::invalid_argument("Incomplete UTF-8 sequence");
            for (unsigned continuation = 0; continuation < count; ++continuation)
            {
                const auto next = static_cast<unsigned char>(text[index++]);
                if ((next & 0xc0) != 0x80) throw std::invalid_argument("Invalid UTF-8 continuation byte");
                value = (value << 6) | (next & 0x3f);
            }
            if (value < minimum || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff)) throw std::invalid_argument("Invalid UTF-8 code point");
            if (value < 0x10000) result.push_back(static_cast<TCHAR>(value));
            else
            {
                value -= 0x10000;
                result.push_back(static_cast<TCHAR>(0xd800 | (value >> 10)));
                result.push_back(static_cast<TCHAR>(0xdc00 | (value & 0x3ff)));
            }
        }
        return result;
    }

    std::string EncodeUtf8(std::basic_string_view<TCHAR> text)
    {
        static_assert(sizeof(TCHAR) == 2);
        std::string result;
        result.reserve(text.size());
        for (std::size_t index = 0; index < text.size(); ++index)
        {
            uint32 value = static_cast<uint16>(text[index]);
            if (value >= 0xd800 && value <= 0xdbff)
            {
                if (++index == text.size()) throw std::invalid_argument("Incomplete UTF-16 surrogate pair");
                uint32 low = static_cast<uint16>(text[index]);
                if (low < 0xdc00 || low > 0xdfff) throw std::invalid_argument("Invalid UTF-16 surrogate pair");
                value = 0x10000 + ((value - 0xd800) << 10) + low - 0xdc00;
            }
            else if (value >= 0xdc00 && value <= 0xdfff) throw std::invalid_argument("Unpaired UTF-16 low surrogate");
            if (value < 0x80) result.push_back(static_cast<char>(value));
            else
            {
                if (value < 0x800) result.push_back(static_cast<char>(0xc0 | (value >> 6)));
                else
                {
                    if (value < 0x10000) result.push_back(static_cast<char>(0xe0 | (value >> 12)));
                    else
                    {
                        result.push_back(static_cast<char>(0xf0 | (value >> 18)));
                        result.push_back(static_cast<char>(0x80 | ((value >> 12) & 0x3f)));
                    }
                    result.push_back(static_cast<char>(0x80 | ((value >> 6) & 0x3f)));
                }
                result.push_back(static_cast<char>(0x80 | (value & 0x3f)));
            }
        }
        return result;
    }
}
