#include "AnyUnreal/RHI/Command/IndirectDraw.h"
#include "AnyUnreal/RHI/Command/MemoryAcquire.h"
#include "prx/libSceAgc/DcbDraw/include/DrawNonIndexed.hpp"
#include "prx/libSceAgc/DcbDraw/include/DrawIndexed.hpp"
#include "prx/libSceAgc/DcbDraw/include/IndexBuffer.hpp"
#include "prx/libSceAgc/DcbDraw/include/Instancing.hpp"
#include <array>
#include <limits>
#include <stdexcept>

namespace AnyUnreal::RHI
{
    void WriteIndirectDraw(CommandBuffer& commands, const Shader::UserDataLayout& layout, const BufferAllocation& arguments, std::uint32_t argumentOffset, const BufferAllocation* indices, std::uint32_t indexStride)
    {
        if (layout.GetStage() != Shader::ShaderStage::Vertex && layout.GetStage() != Shader::ShaderStage::Geometry) throw std::invalid_argument("Indirect draw requires vertex shader metadata");
        if (argumentOffset % 4 != 0) throw std::invalid_argument("Indirect draw offset must be aligned to four bytes");
        arguments.GetRange(argumentOffset, indices ? 20 : 16);
        if (indices)
        {
            if (indexStride != 2 && indexStride != 4) throw std::invalid_argument("Indirect index stride must be two or four bytes");
            if (indices->Size() % indexStride != 0 || indices->Size() / indexStride > std::numeric_limits<std::uint32_t>::max()) throw std::out_of_range("Indirect index buffer extent is invalid");
        }
        else if (indexStride != 0) throw std::invalid_argument("Index stride supplied without an index buffer");
        std::uint64_t modifier = 0;
        if (const auto slot = layout.GetBaseVertexSlot()) modifier |= 1u | (static_cast<std::uint64_t>(*slot) << 9);
        if (const auto slot = layout.GetStartInstanceSlot()) modifier |= 4u | (static_cast<std::uint64_t>(*slot) << 19);
        const std::array ranges{GpuMemoryRange{arguments.Data(), arguments.Size()}};
        WriteMemoryAcquire(commands, ranges);
        sceAgcDcbSetBaseIndirectArgs(&commands, 0, arguments.Data());
        if (indices)
        {
            const std::array indexRanges{GpuMemoryRange{indices->Data(), indices->Size()}};
            WriteMemoryAcquire(commands, indexRanges);
            sceAgcDcbSetIndexBuffer(&commands, reinterpret_cast<std::uintptr_t>(indices->Data()));
            sceAgcDcbSetIndexCount(&commands, static_cast<std::uint32_t>(indices->Size() / indexStride));
            sceAgcDcbSetIndexSize(&commands, indexStride == 2 ? 0 : 1, 0);
            sceAgcDcbDrawIndexIndirect(&commands, argumentOffset, modifier);
        }
        else sceAgcDcbDrawIndirect(&commands, argumentOffset, modifier);
    }
}
