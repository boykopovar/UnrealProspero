#include <oodle2.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <vector>

int main()
{
    constexpr std::array codecs = {OodleLZ_Compressor_Kraken, OodleLZ_Compressor_Mermaid, OodleLZ_Compressor_Selkie, OodleLZ_Compressor_Leviathan};
    constexpr std::array levels = {OodleLZ_CompressionLevel_Fast, OodleLZ_CompressionLevel_Normal, OodleLZ_CompressionLevel_Optimal1};
    constexpr std::array<std::size_t, 4> sizes = {1, 255, 4097, 262144};
    unsigned checks = 0;
    for (std::size_t size : sizes)
    {
        for (unsigned pattern = 0; pattern < 4; ++pattern)
        {
            std::vector<std::uint8_t> input(size);
            std::uint32_t random = 0x183a74d5;
            for (std::size_t index = 0; index < size; ++index)
            {
                random ^= random << 13;
                random ^= random >> 17;
                random ^= random << 5;
                input[index] = static_cast<std::uint8_t>(pattern == 0 ? 0 : pattern == 1 ? index : pattern == 2 ? random : (index * 17 + index / 73) % 251);
            }
            for (auto codec : codecs)
            {
                const auto capacity = OodleLZ_GetCompressedBufferSizeNeeded(codec, size);
                if (capacity <= 0) throw std::runtime_error("Invalid Oodle compression bound");
                std::vector<std::uint8_t> packed(static_cast<std::size_t>(capacity));
                for (auto level : levels)
                {
                    const auto compressed = OodleLZ_Compress(codec, input.data(), size, packed.data(), level);
                    if (compressed <= 0 || compressed > capacity) throw std::runtime_error("Oodle compression failed");
                    std::vector<std::uint8_t> output(size, 0x9a);
                    if (OodleLZ_Decompress(packed.data(), compressed, output.data(), size) != static_cast<OO_SINTa>(size) || output != input) throw std::runtime_error("Oodle round trip failed");
                    ++checks;
                }
            }
        }
    }
    if (std::printf("Oodle round trips passed: %u\n", checks) < 0) throw std::runtime_error("Cannot write Oodle results");
    return 0;
}
