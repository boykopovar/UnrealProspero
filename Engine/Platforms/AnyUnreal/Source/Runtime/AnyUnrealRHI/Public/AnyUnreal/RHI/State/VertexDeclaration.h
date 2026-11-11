#ifndef ANYUNREAL_RHI_VERTEX_DECLARATION_H
#define ANYUNREAL_RHI_VERTEX_DECLARATION_H

#include "RHIResources.h"
#include "RHI.h"
#include "AnyUnreal/RHI/Descriptor/BufferDescriptor.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API VertexDeclaration final : public FRHIVertexDeclaration
    {
    public:
        explicit VertexDeclaration(const FVertexDeclarationElementList& elements);
        bool GetInitializer(FVertexDeclarationElementList& initializer) override;
        const FVertexDeclarationElementList& GetElements() const;
        static uint32 GetElementSize(EVertexElementType type);
        static VertexFormat GetElementFormat(EVertexElementType type);

    private:
        FVertexDeclarationElementList elements;
    };
}

#endif
