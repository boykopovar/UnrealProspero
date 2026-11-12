#include "UnrealProspero/SaveData/SlotFile.h"
#include "UnrealProspero/Kernel/File.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>

namespace UnrealProspero::SaveData
{
    namespace
    {
        constexpr std::array<std::byte, 8> signature{std::byte{'U'}, std::byte{'P'}, std::byte{'S'}, std::byte{'G'}, std::byte{1}, std::byte{0}, std::byte{0}, std::byte{0}};
        using Header = std::array<std::byte, 20>;

        void encode(std::span<std::byte> bytes, std::uint64_t value)
        {
            for (auto& byte : bytes)
            {
                byte = static_cast<std::byte>(value & 255);
                value >>= 8;
            }
            if (value != 0) throw std::overflow_error("SaveGame integer exceeds format range");
        }

        std::uint64_t decode(std::span<const std::byte> bytes)
        {
            std::uint64_t value = 0;
            for (std::size_t index = 0; index < bytes.size(); ++index) value |= static_cast<std::uint64_t>(bytes[index]) << (index * 8);
            return value;
        }

        void validateName(std::string_view name)
        {
            if (name.empty() || name.find('\0') != std::string_view::npos || name.size() > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) throw std::invalid_argument("Invalid SaveGame slot name");
        }

        struct Contents
        {
            std::string Name;
            std::size_t Size;
        };

        Contents readHeader(Kernel::File& file)
        {
            Header header{};
            if (file.Read(header) != header.size() || !std::equal(signature.begin(), signature.end(), header.begin())) throw std::runtime_error("Invalid SaveGame header");
            const auto nameSize = decode(std::span(header).subspan(8, 4));
            const auto dataSize = decode(std::span(header).subspan(12, 8));
            constexpr auto limit = static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max());
            if (nameSize == 0 || nameSize > limit || dataSize > limit) throw std::runtime_error("Invalid SaveGame lengths");
            if (static_cast<std::uint64_t>(file.Size()) != header.size() + nameSize + dataSize) throw std::runtime_error("SaveGame size does not match its header");
            std::string name(static_cast<std::size_t>(nameSize), '\0');
            if (file.Read(std::as_writable_bytes(std::span(name.data(), name.size()))) != name.size()) throw std::runtime_error("Truncated SaveGame slot name");
            validateName(name);
            return {std::move(name), static_cast<std::size_t>(dataSize)};
        }
    }

    void SlotFile::Write(std::string_view path, std::string_view name, std::span<const std::byte> data)
    {
        validateName(name);
        if (data.size() > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) throw std::overflow_error("SaveGame payload exceeds Unreal array range");
        Header header{};
        std::copy(signature.begin(), signature.end(), header.begin());
        encode(std::span(header).subspan(8, 4), name.size());
        encode(std::span(header).subspan(12, 8), data.size());
        Kernel::File file(path, Kernel::FileAccess::Write, Kernel::FileCreation::CreateOrTruncate);
        file.Write(header);
        file.Write(std::as_bytes(std::span(name.data(), name.size())));
        file.Write(data);
        file.Flush();
        file.Close();
    }

    std::string SlotFile::ReadName(std::string_view path)
    {
        Kernel::File file(path, Kernel::FileAccess::Read);
        auto contents = readHeader(file);
        file.Close();
        return std::move(contents.Name);
    }

    std::vector<std::byte> SlotFile::Read(std::string_view path, std::string_view expectedName)
    {
        validateName(expectedName);
        Kernel::File file(path, Kernel::FileAccess::Read);
        const auto contents = readHeader(file);
        if (contents.Name != expectedName) throw std::runtime_error("SaveGame slot name collision");
        std::vector<std::byte> data(contents.Size);
        if (file.Read(data) != data.size()) throw std::runtime_error("Truncated SaveGame payload");
        file.Close();
        return data;
    }
}
