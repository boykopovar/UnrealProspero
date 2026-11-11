#include "AnyUnreal/Shader/DescriptorLayout.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <stdexcept>

int main(int argumentCount, char** arguments)
{
    try
    {
        if (argumentCount != 2) throw std::invalid_argument("Expected a SPIR-V file");
        std::ifstream input(arguments[1], std::ios::binary);
        if (!input) throw std::runtime_error("Cannot open SPIR-V file");
        const std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
        if (input.bad() || bytes.size() % 4 != 0) throw std::runtime_error("Cannot read SPIR-V words");
        std::vector<std::uint32_t> words(bytes.size() / 4);
        for (std::size_t index = 0; index < words.size(); ++index)
        {
            for (unsigned byte = 0; byte < 4; ++byte) words[index] |= static_cast<std::uint32_t>(bytes[index * 4 + byte]) << (byte * 8);
        }
        const auto bindings = AnyUnreal::Shader::ReflectDescriptorLayout(words, "main");
        try
        {
            AnyUnreal::Shader::ReflectDescriptorLayout(words, "missingEntryPoint");
            throw std::runtime_error("Missing entry point was accepted");
        }
        catch (const std::invalid_argument&)
        {
        }
        for (const auto& binding : bindings)
        {
            std::cout << binding.Set << ',' << binding.Binding << ',' << binding.Type << ',' << binding.Count << ',' << binding.Offset << ',' << binding.Stride << '\n';
            std::cerr << std::quoted(binding.Name) << ' ' << static_cast<int>(binding.Kind) << ' ' << binding.BlockSize << '\n';
            for (const auto& member : binding.Members) std::cerr << std::quoted(member.Name) << ' ' << member.Offset << ' ' << member.Size << '\n';
        }
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
