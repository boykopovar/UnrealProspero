#include <zlib.h>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <vector>

int main()
{
    constexpr std::array<std::size_t, 5> sizes = {0, 1, 255, 4097, 262144};
    unsigned checks = 0;
    for (std::size_t size : sizes)
    {
        std::vector<std::uint8_t> input(size);
        for (std::size_t index = 0; index < size; ++index) input[index] = static_cast<std::uint8_t>((index * 17 + index / 73) % 251);
        std::vector<std::uint8_t> packed(compressBound(size));
        for (int level = Z_NO_COMPRESSION; level <= Z_BEST_COMPRESSION; ++level)
        {
            uLongf compressed = packed.size();
            if (compress2(packed.data(), &compressed, input.data(), size, level) != Z_OK) throw std::runtime_error("zlib compression failed");
            std::vector<std::uint8_t> output(size + 1, 0x9a);
            uLongf restored = output.size();
            if (uncompress(output.data(), &restored, packed.data(), compressed) != Z_OK || restored != size) throw std::runtime_error("zlib decompression failed");
            output.resize(size);
            if (output != input) throw std::runtime_error("zlib round trip differs");
            ++checks;
        }
    }
    constexpr auto path = "zlib-roundtrip.gz";
    if (auto* existing = std::fopen(path, "rb"))
    {
        std::fclose(existing);
        throw std::runtime_error("zlib test file already exists");
    }
    if (errno != ENOENT) throw std::runtime_error("Cannot inspect zlib test file");
    std::array<std::uint8_t, 4097> fileInput{};
    for (std::size_t index = 0; index < fileInput.size(); ++index) fileInput[index] = static_cast<std::uint8_t>(index);
    gzFile writer = gzopen(path, "wb");
    if (!writer) throw std::runtime_error("Cannot create gzip stream");
    const int written = gzwrite(writer, fileInput.data(), fileInput.size());
    const int writeClose = gzclose(writer);
    if (written != static_cast<int>(fileInput.size()) || writeClose != Z_OK) throw std::runtime_error("Cannot write gzip stream");
    gzFile reader = gzopen(path, "rb");
    if (!reader) throw std::runtime_error("Cannot open gzip stream");
    std::array<std::uint8_t, 4097> fileOutput{};
    const int read = gzread(reader, fileOutput.data(), fileOutput.size());
    const int end = gzgetc(reader);
    const int readClose = gzclose(reader);
    if (read != static_cast<int>(fileInput.size()) || fileInput != fileOutput || end != -1 || readClose != Z_OK) throw std::runtime_error("Gzip file round trip differs");
    if (std::remove(path) != 0) throw std::runtime_error("Cannot remove gzip test file");
    ++checks;
    if (std::printf("zlib round trips passed: %u\n", checks) < 0) throw std::runtime_error("Cannot write zlib results");
    return 0;
}
