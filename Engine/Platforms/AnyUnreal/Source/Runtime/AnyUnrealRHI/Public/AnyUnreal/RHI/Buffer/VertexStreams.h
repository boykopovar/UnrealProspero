#ifndef ANYUNREAL_RHI_VERTEX_STREAMS_H
#define ANYUNREAL_RHI_VERTEX_STREAMS_H

#include "AnyUnreal/RHI/Buffer/Buffer.h"
#include "AnyUnreal/RHI/Descriptor/VertexBufferTable.h"
#include <map>

namespace AnyUnreal::RHI
{
    class VertexDeclaration;

    class ANYUNREALRHI_API VertexStreams final
    {
    public:
        struct Snapshot
        {
            std::shared_ptr<BufferAllocation> Storage;
            std::size_t Offset;
        };

        void Set(uint32 index, FRHIBuffer* buffer, uint32 offset);
        Snapshot Capture(uint32 index) const;
        std::unique_ptr<VertexBufferTable> CaptureTable(const VertexDeclaration& declaration, const Shader::UserDataLayout& layout) const;

    private:
        struct Binding
        {
            TRefCountPtr<Buffer> Resource;
            uint32 Offset;
        };

        std::map<uint32, Binding> bindings;
    };
}

#endif
