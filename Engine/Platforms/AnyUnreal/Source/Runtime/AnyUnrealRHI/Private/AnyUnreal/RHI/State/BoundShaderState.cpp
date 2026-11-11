#include "AnyUnreal/RHI/State/BoundShaderState.h"
#include "AnyUnreal/RHI/Shader/VertexShader.h"
#include "AnyUnreal/RHI/Shader/GeometryShader.h"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    BoundShaderState::BoundShaderState(FRHIVertexDeclaration* declaration, FRHIVertexShader* vertex, FRHIPixelShader* pixel, FRHIGeometryShader* geometry) : declaration(declaration), vertex(vertex), pixel(pixel), geometry(geometry)
    {
        if (!vertex) throw std::invalid_argument("Bound shader state requires a vertex shader");
        if (geometry) static_cast<GeometryShader*>(geometry)->GetCode().ValidateVertex(static_cast<VertexShader*>(vertex)->GetCode());
    }

    FRHIVertexDeclaration* BoundShaderState::GetVertexDeclaration() const { return declaration.GetReference(); }
    FRHIVertexShader* BoundShaderState::GetVertexShader() const { return vertex.GetReference(); }
    FRHIPixelShader* BoundShaderState::GetPixelShader() const { return pixel.GetReference(); }
    FRHIGeometryShader* BoundShaderState::GetGeometryShader() const { return geometry.GetReference(); }
}
