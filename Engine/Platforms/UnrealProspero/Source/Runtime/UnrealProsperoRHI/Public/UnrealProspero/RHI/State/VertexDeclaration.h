#ifndef UNREALPROSPERO_RHI_VERTEX_DECLARATION_H
#define UNREALPROSPERO_RHI_VERTEX_DECLARATION_H

#include "RHIResources.h"
#include "RHI.h"
#include "UnrealProspero/RHI/Descriptor/BufferDescriptor.h"

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API VertexDeclaration final : public FRHIVertexDeclaration
    {
    public:
        explicit VertexDeclaration(const FVertexDeclarationElementList& elements);
        bool GetInitializer(FVertexDeclarationElementList& initializer) override;
        const FVertexDeclarationElementList& GetElements() const;
        static uint32 GetElementSize(EVertexElementType type);
        static BufferFormat GetElementFormat(EVertexElementType type);

    private:
        FVertexDeclarationElementList elements;
    };
}

#endif
