#include "AnyUnreal/Text/Encoding.h"
#include <stdexcept>

namespace AnyUnreal::Text
{
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
