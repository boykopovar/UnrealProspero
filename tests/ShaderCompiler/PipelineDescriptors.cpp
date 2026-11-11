#include "UnrealProspero/Shader/PipelineDescriptors.h"
#include "UnrealProspero/Shader/EntryPoint.h"
#include <array>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace
{
    std::vector<std::uint32_t> read(const char* path)
    {
        std::ifstream input(path, std::ios::binary);
        if (!input) throw std::runtime_error("Cannot open SPIR-V input");
        const std::vector<unsigned char> bytes(std::istreambuf_iterator<char>(input), {});
        if (input.bad() || bytes.size() < 20 || bytes.size() % 4 != 0) throw std::runtime_error("Invalid SPIR-V input");
        std::vector<std::uint32_t> words(bytes.size() / 4);
        for (std::size_t index = 0; index < bytes.size(); ++index) words[index / 4] |= static_cast<std::uint32_t>(bytes[index]) << ((index % 4) * 8);
        return words;
    }

    void write(const char* path, const std::vector<std::uint32_t>& words)
    {
        std::ofstream output(path, std::ios::binary);
        if (!output) throw std::runtime_error("Cannot open remapped SPIR-V output");
        for (const auto word : words)
        {
            for (unsigned shift = 0; shift < 32; shift += 8) output.put(static_cast<char>(word >> shift));
        }
        output.close();
        if (!output) throw std::runtime_error("Cannot write remapped SPIR-V output");
    }
}

int main(int count, char** arguments)
{
    using namespace UnrealProspero::Shader;
    if (count != 5) throw std::invalid_argument("Expected vertex and geometry input and output paths");
    auto vertex = read(arguments[1]);
    auto geometry = read(arguments[2]);
    for (const auto& name : {"V", "MainVS", "GeometryEntryPoint"})
    {
        const auto renamed = RenameEntryPoint(vertex, "main", name);
        if (RenameEntryPoint(renamed, name, "main") != vertex || ReflectDescriptorLayout(renamed, name).size() != 1) throw std::runtime_error("Entry point renaming changed shader interfaces");
    }
    bool missingRejected = false;
    try { RenameEntryPoint(vertex, "MissingEntryPoint", "main"); }
    catch (const std::invalid_argument&) { missingRejected = true; }
    if (!missingRejected) throw std::runtime_error("Missing entry point was accepted");
    auto vertexLayout = ReflectDescriptorLayout(vertex, "main");
    auto geometryLayout = ReflectDescriptorLayout(geometry, "main");
    if (vertexLayout.size() != 1 || geometryLayout.size() != 1 || vertexLayout[0].Binding != 0 || geometryLayout[0].Binding != 0) throw std::runtime_error("Expected colliding pipeline bindings");
    const auto originalVertex = vertex;
    const auto originalGeometry = geometry;
    geometryLayout[0].Count = 2;
    bool rejected = false;
    try { MergePipelineDescriptors(std::array{PipelineStageDescriptors{vertex, vertexLayout, "main"}, PipelineStageDescriptors{geometry, geometryLayout, "main"}}); }
    catch (const std::invalid_argument&) { rejected = true; }
    if (!rejected || vertex != originalVertex || geometry != originalGeometry || vertexLayout[0].Binding != 0) throw std::runtime_error("Rejected pipeline changed its inputs");
    geometryLayout[0].Count = 1;
    const auto merged = MergePipelineDescriptors(std::array{PipelineStageDescriptors{vertex, vertexLayout, "main"}, PipelineStageDescriptors{geometry, geometryLayout, "main"}});
    if (merged.size() != 2 || merged[0].Binding != 0 || merged[0].Offset != 0 || merged[1].Binding != 1 || merged[1].Offset != 16 || vertexLayout[0].Binding != 0 || geometryLayout[0].Binding != 1 || geometryLayout[0].Offset != 16) throw std::runtime_error("Pipeline descriptor remapping is incorrect");
    if (ReflectDescriptorLayout(vertex, "main")[0].Binding != 0 || ReflectDescriptorLayout(geometry, "main")[0].Binding != 1) throw std::runtime_error("SPIR-V bindings were not remapped");
    write(arguments[3], vertex);
    write(arguments[4], geometry);
    std::cout << "Pipeline descriptor isolation passed\n";
}
