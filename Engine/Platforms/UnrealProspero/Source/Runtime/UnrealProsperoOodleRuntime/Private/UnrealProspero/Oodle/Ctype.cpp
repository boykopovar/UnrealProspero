#include "UnrealProspero/Oodle/Ctype.h"
#include <array>
#include <bit>
#include <cctype>
#include <cstdio>

namespace
{
    static_assert(EOF == -1);
    static_assert(std::endian::native == std::endian::little || std::endian::native == std::endian::big);
    constexpr int tableBegin = -128;
    constexpr int tableEnd = 256;

    template<class TValue>
    struct Table
    {
        std::array<TValue, tableEnd - tableBegin> values{};
        const TValue* pointer = values.data() - tableBegin;
    };

    int character(int index)
    {
        return index == EOF ? EOF : static_cast<unsigned char>(index);
    }

    std::uint16_t flag(unsigned index)
    {
        if constexpr (std::endian::native == std::endian::big) return static_cast<std::uint16_t>(1u << index);
        return static_cast<std::uint16_t>(index < 8 ? 1u << (index + 8) : 1u << (index - 8));
    }
}

extern "C" const std::uint16_t** UnrealProsperoOodleCtype()
{
    thread_local Table<std::uint16_t> table;
    for (int index = tableBegin; index < tableEnd; ++index)
    {
        const int value = character(index);
        const std::array<bool, 12> properties = {std::isupper(value) != 0, std::islower(value) != 0, std::isalpha(value) != 0, std::isdigit(value) != 0, std::isxdigit(value) != 0, std::isspace(value) != 0, std::isprint(value) != 0, std::isgraph(value) != 0, std::isblank(value) != 0, std::iscntrl(value) != 0, std::ispunct(value) != 0, std::isalnum(value) != 0};
        std::uint16_t result = 0;
        for (unsigned property = 0; property < properties.size(); ++property) if (properties[property]) result |= flag(property);
        table.values[index - tableBegin] = result;
    }
    return &table.pointer;
}

extern "C" const std::int32_t** UnrealProsperoOodleLowercase()
{
    thread_local Table<std::int32_t> table;
    for (int index = tableBegin; index < tableEnd; ++index) table.values[index - tableBegin] = std::tolower(character(index));
    return &table.pointer;
}

extern "C" const std::int32_t** UnrealProsperoOodleUppercase()
{
    thread_local Table<std::int32_t> table;
    for (int index = tableBegin; index < tableEnd; ++index) table.values[index - tableBegin] = std::toupper(character(index));
    return &table.pointer;
}
