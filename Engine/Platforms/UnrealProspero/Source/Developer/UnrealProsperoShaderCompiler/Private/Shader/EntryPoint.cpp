#include "UnrealProspero/Shader/EntryPoint.h"
#include <limits>
#include <stdexcept>
#include <string>

namespace UnrealProspero::Shader
{
    std::vector<std::uint32_t> RenameEntryPoint(std::span<const std::uint32_t> words, std::string_view original, std::string_view replacement)
    {
        if (words.size() < 5 || words[0] != 0x07230203u || words[3] == 0 || words[4] != 0) throw std::invalid_argument("Invalid SPIR-V entry point input");
        if (original.empty() || replacement.empty() || original.find('\0') != std::string_view::npos || replacement.find('\0') != std::string_view::npos) throw std::invalid_argument("Invalid SPIR-V entry point name");
        if (replacement.size() > (std::numeric_limits<std::uint16_t>::max() - 3u) * 4u - 1u) throw std::overflow_error("SPIR-V entry point name is too long");
        std::vector<std::uint32_t> result(words.begin(), words.begin() + 5);
        bool found = false;
        for (std::size_t offset = 5; offset < words.size();)
        {
            const auto count = words[offset] >> 16;
            const auto opcode = words[offset] & 0xffffu;
            if (count == 0 || count > words.size() - offset) throw std::invalid_argument("Invalid SPIR-V instruction length");
            if (opcode != 15)
            {
                result.insert(result.end(), words.begin() + offset, words.begin() + offset + count);
                offset += count;
                continue;
            }
            if (count < 4 || words[offset + 2] == 0 || words[offset + 2] >= words[3]) throw std::invalid_argument("Invalid SPIR-V entry point instruction");
            std::string name;
            bool terminated = false;
            std::size_t nameWords = 0;
            for (std::size_t index = 3; index < count && !terminated; ++index)
            {
                const auto word = words[offset + index];
                ++nameWords;
                for (unsigned shift = 0; shift < 32; shift += 8)
                {
                    const auto character = static_cast<char>((word >> shift) & 0xffu);
                    if (terminated && character != 0) throw std::invalid_argument("Invalid SPIR-V string padding");
                    if (character == 0) terminated = true;
                    else name.push_back(character);
                }
            }
            if (!terminated || name.empty()) throw std::invalid_argument("Unterminated SPIR-V entry point name");
            if (name == replacement && name != original) throw std::invalid_argument("SPIR-V entry point name already exists");
            if (name == original)
            {
                if (found) throw std::invalid_argument("Ambiguous SPIR-V entry point name");
                found = true;
                const auto replacementWords = (replacement.size() + 4) / 4;
                const auto newCount = count - nameWords + replacementWords;
                if (newCount > std::numeric_limits<std::uint16_t>::max()) throw std::overflow_error("SPIR-V entry point instruction is too long");
                result.push_back((static_cast<std::uint32_t>(newCount) << 16) | opcode);
                result.push_back(words[offset + 1]);
                result.push_back(words[offset + 2]);
                const auto nameOffset = result.size();
                result.resize(nameOffset + replacementWords, 0);
                for (std::size_t index = 0; index < replacement.size(); ++index) result[nameOffset + index / 4] |= static_cast<std::uint32_t>(static_cast<unsigned char>(replacement[index])) << ((index % 4) * 8);
                result.insert(result.end(), words.begin() + offset + 3 + nameWords, words.begin() + offset + count);
            }
            else result.insert(result.end(), words.begin() + offset, words.begin() + offset + count);
            offset += count;
        }
        if (!found) throw std::invalid_argument("SPIR-V entry point does not exist");
        return result;
    }
}
