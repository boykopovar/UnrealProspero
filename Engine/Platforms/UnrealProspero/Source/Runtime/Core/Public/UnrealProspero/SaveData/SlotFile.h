#ifndef UNREALPROSPERO_SAVE_DATA_SLOT_FILE_H
#define UNREALPROSPERO_SAVE_DATA_SLOT_FILE_H

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace UnrealProspero::SaveData
{
    class SlotFile final
    {
    public:
        static void Write(std::string_view path, std::string_view name, std::span<const std::byte> data);
        static std::string ReadName(std::string_view path);
        static std::vector<std::byte> Read(std::string_view path, std::string_view expectedName);
    };
}

#endif
