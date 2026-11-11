#include "AnyUnreal/RHI/Command/CommandStorage.h"
#include "AnyUnreal/RHI/Command/IndirectDraw.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libSceAgc/libSceAgc.hpp"
#include "prx/libkernel/include/Api.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string_view>

int main()
{
    if (getargc_nid_postfix() != 3) throw std::invalid_argument("Expected metadata path and vertex or geometry stage");
    const auto* arguments = getargv_nid_postfix();
    const std::string_view stageName(arguments[2]);
    if (stageName != "vertex" && stageName != "geometry") throw std::invalid_argument("Invalid indirect draw stage");
    std::ifstream file(arguments[1], std::ios::binary);
    if (!file) throw std::runtime_error("Cannot open shader metadata");
    const std::vector<std::uint8_t> bytes(std::istreambuf_iterator<char>(file), {});
    if (file.bad()) throw std::runtime_error("Cannot read shader metadata");
    namespace RHI = AnyUnreal::RHI;
    namespace Shader = AnyUnreal::Shader;
    const auto stage = stageName == "vertex" ? Shader::ShaderStage::Vertex : Shader::ShaderStage::Geometry;
    const auto layout = Shader::UserDataLayout::Read(bytes, stage);
    AnyUnreal::Kernel::CheckResult(sceAgcInit(11), "sceAgcInit");
    RHI::BufferAllocation argumentBuffer(64, layout.GetAddressHigh());
    RHI::BufferAllocation indices(64, layout.GetAddressHigh());
    for (const auto stride : {0u, 2u, 4u})
    {
        RHI::CommandStorage storage(4096, layout.GetAddressHigh());
        auto& commands = storage.Get();
        const auto reject = [&](auto action)
        {
            const auto* before = commands.cursor_up;
            bool rejected = false;
            try { action(); }
            catch (const std::exception&) { rejected = true; }
            if (!rejected || commands.cursor_up != before) throw std::runtime_error("Invalid indirect draw changed commands");
        };
        reject([&] { RHI::WriteIndirectDraw(commands, layout, argumentBuffer, 1, stride ? &indices : nullptr, stride); });
        reject([&] { RHI::WriteIndirectDraw(commands, layout, argumentBuffer, 60, stride ? &indices : nullptr, stride); });
        reject([&] { RHI::WriteIndirectDraw(commands, layout, argumentBuffer, 16, &indices, 3); });
        RHI::WriteIndirectDraw(commands, layout, argumentBuffer, 16, stride ? &indices : nullptr, stride);
        unsigned draws = 0;
        for (const auto* packet = commands.bottom; packet < commands.cursor_up;)
        {
            if ((*packet >> 30) != 3) throw std::runtime_error("Unexpected command packet type");
            const auto size = ((*packet >> 16) & 0x3fff) + 2;
            if (size > static_cast<std::size_t>(commands.cursor_up - packet)) throw std::runtime_error("Truncated indirect draw packet");
            const auto opcode = (*packet >> 8) & 0xff;
            if (opcode == 0x24 || opcode == 0x25)
            {
                ++draws;
                const auto vertexSlot = layout.GetBaseVertexSlot();
                const auto instanceSlot = layout.GetStartInstanceSlot();
                const auto expectedVertex = vertexSlot ? 0x8cu + *vertexSlot : 0x280u;
                const auto expectedInstance = instanceSlot ? 0x8cu + *instanceSlot : 0x280u;
                if (size != 5 || opcode != (stride ? 0x25u : 0x24u) || packet[1] != 16 || packet[2] != expectedVertex || packet[3] != expectedInstance || packet[4] != (stride ? 0u : 2u)) throw std::runtime_error("Indirect draw patches the wrong shader registers");
            }
            packet += size;
        }
        if (draws != 1) throw std::runtime_error("Indirect draw packet is missing or duplicated");
    }
    std::cout << "Indirect draw arguments and shader register offsets verified\n";
}
