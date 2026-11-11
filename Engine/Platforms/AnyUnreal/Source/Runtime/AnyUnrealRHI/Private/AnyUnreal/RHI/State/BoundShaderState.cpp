#include "AnyUnreal/RHI/State/BoundShaderState.h"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    BoundShaderState::BoundShaderState(FRHIVertexDeclaration* declaration, FRHIVertexShader* vertex, FRHIPixelShader* pixel, FRHIGeometryShader* geometry) : declaration(declaration), vertex(vertex), pixel(pixel), geometry(geometry)
    {
        if (!vertex) throw std::invalid_argument("Bound shader state requires a vertex shader");
    }

    FRHIVertexDeclaration* BoundShaderState::GetVertexDeclaration() const { return declaration.GetReference(); }
    FRHIVertexShader* BoundShaderState::GetVertexShader() const { return vertex.GetReference(); }
    FRHIPixelShader* BoundShaderState::GetPixelShader() const { return pixel.GetReference(); }
    FRHIGeometryShader* BoundShaderState::GetGeometryShader() const { return geometry.GetReference(); }
}
