#include "UnrealProspero/RHI/State/VertexDeclaration.h"
#include <array>
#include <stdexcept>

namespace UnrealProspero::RHI
{
    uint32 VertexDeclaration::GetElementSize(EVertexElementType type)
    {
        switch (type)
        {
        case VET_Float1:
        case VET_PackedNormal:
        case VET_UByte4:
        case VET_UByte4N:
        case VET_Color:
        case VET_Short2:
        case VET_Short2N:
        case VET_Half2:
        case VET_UShort2:
        case VET_UShort2N:
        case VET_URGB10A2N:
        case VET_UInt:
            return 4;
        case VET_Float2:
        case VET_Short4:
        case VET_Half4:
        case VET_Short4N:
        case VET_UShort4:
        case VET_UShort4N:
            return 8;
        case VET_Float3:
            return 12;
        case VET_Float4:
            return 16;
        default:
            throw std::invalid_argument("Invalid vertex element type");
        }
    }

    BufferFormat VertexDeclaration::GetElementFormat(EVertexElementType type)
    {
        switch (type)
        {
        case VET_Float1: return BufferFormat::Float1;
        case VET_Float2: return BufferFormat::Float2;
        case VET_Float3: return BufferFormat::Float3;
        case VET_Float4: return BufferFormat::Float4;
        case VET_PackedNormal: return BufferFormat::Byte4Snorm;
        case VET_UByte4: return BufferFormat::Byte4Uint;
        case VET_UByte4N: return BufferFormat::Byte4Unorm;
        case VET_Color: return BufferFormat::Bgra8Unorm;
        case VET_Short2: return BufferFormat::Short2Sint;
        case VET_Short4: return BufferFormat::Short4Sint;
        case VET_Short2N: return BufferFormat::Short2Snorm;
        case VET_Half2: return BufferFormat::Half2;
        case VET_Half4: return BufferFormat::Half4;
        case VET_Short4N: return BufferFormat::Short4Snorm;
        case VET_UShort2: return BufferFormat::Short2Uint;
        case VET_UShort4: return BufferFormat::Short4Uint;
        case VET_UShort2N: return BufferFormat::Short2Unorm;
        case VET_UShort4N: return BufferFormat::Short4Unorm;
        case VET_URGB10A2N: return BufferFormat::Rgb10A2Unorm;
        case VET_UInt: return BufferFormat::Uint1;
        default: throw std::invalid_argument("Invalid vertex element type");
        }
    }

    VertexDeclaration::VertexDeclaration(const FVertexDeclarationElementList& elements) : elements(elements)
    {
        if (elements.Num() > MaxVertexElementCount) throw std::out_of_range("Vertex declaration exceeds the attribute limit");
        std::array<bool, MaxVertexElementCount> attributes{};
        for (int32 index = 0; index < elements.Num(); ++index)
        {
            const auto& element = elements[index];
            GetElementSize(element.Type);
            if (element.AttributeIndex >= attributes.size()) throw std::out_of_range("Vertex attribute index exceeds the attribute limit");
            if (attributes[element.AttributeIndex]) throw std::invalid_argument("Vertex attribute is declared more than once");
            if (element.bUseInstanceIndex > 1) throw std::invalid_argument("Invalid vertex instance index flag");
            attributes[element.AttributeIndex] = true;
            for (int32 previous = 0; previous < index; ++previous)
            {
                const auto& other = elements[previous];
                if (other.StreamIndex == element.StreamIndex && (other.Stride != element.Stride || other.bUseInstanceIndex != element.bUseInstanceIndex)) throw std::invalid_argument("Vertex stream elements disagree on stride or input rate");
            }
        }
    }

    bool VertexDeclaration::GetInitializer(FVertexDeclarationElementList& initializer)
    {
        initializer = elements;
        return true;
    }

    const FVertexDeclarationElementList& VertexDeclaration::GetElements() const
    {
        return elements;
    }
}
