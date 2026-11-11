#include "AnyUnreal/RHI/Memory/HalfFloat.h"
#include <bit>
#include <limits>

namespace AnyUnreal::RHI
{
    static_assert(sizeof(float) == sizeof(std::uint32_t) && std::numeric_limits<float>::is_iec559);

    std::uint16_t EncodeHalfFloat(float value)
    {
        const auto bits = std::bit_cast<std::uint32_t>(value);
        const auto sign = (bits >> 16) & 0x8000u;
        const auto exponent = (bits >> 23) & 0xffu;
        const auto fraction = bits & 0x7fffffu;
        if (exponent == 255) return static_cast<std::uint16_t>(sign | 0x7c00u | (fraction == 0 ? 0u : (fraction >> 13) | 0x200u));
        if (exponent >= 143) return static_cast<std::uint16_t>(sign | 0x7c00u);
        if (exponent < 102) return static_cast<std::uint16_t>(sign);
        if (exponent < 113)
        {
            const auto significand = fraction | 0x800000u;
            const auto shift = 126 - exponent;
            const auto result = significand >> shift;
            const auto remainder = significand & ((1u << shift) - 1);
            const auto halfway = 1u << (shift - 1);
            return static_cast<std::uint16_t>(sign | (result + (remainder > halfway || (remainder == halfway && (result & 1)))));
        }
        const auto rounded = fraction + 0xfffu + ((fraction >> 13) & 1);
        return static_cast<std::uint16_t>(sign | (((exponent - 112) << 10) + (rounded >> 13)));
    }

    float DecodeHalfFloat(std::uint16_t value)
    {
        const auto sign = static_cast<std::uint32_t>(value & 0x8000u) << 16;
        auto exponent = static_cast<std::uint32_t>((value >> 10) & 31);
        auto fraction = static_cast<std::uint32_t>(value & 0x3ffu);
        if (exponent == 31) return std::bit_cast<float>(sign | 0x7f800000u | (fraction << 13));
        if (exponent == 0)
        {
            if (fraction == 0) return std::bit_cast<float>(sign);
            exponent = 113;
            while ((fraction & 0x400u) == 0)
            {
                fraction <<= 1;
                --exponent;
            }
            fraction &= 0x3ffu;
        }
        else exponent += 112;
        return std::bit_cast<float>(sign | (exponent << 23) | (fraction << 13));
    }
}
