#ifndef UNREALPROSPERO_RHI_BOUND_SHADER_STATE_H
#define UNREALPROSPERO_RHI_BOUND_SHADER_STATE_H

#include "RHIResources.h"

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API BoundShaderState final : public FRHIBoundShaderState
    {
    public:
        BoundShaderState(FRHIVertexDeclaration* declaration, FRHIVertexShader* vertex, FRHIPixelShader* pixel, FRHIGeometryShader* geometry);
        FRHIVertexDeclaration* GetVertexDeclaration() const;
        FRHIVertexShader* GetVertexShader() const;
        FRHIPixelShader* GetPixelShader() const;
        FRHIGeometryShader* GetGeometryShader() const;

    private:
        FVertexDeclarationRHIRef declaration;
        FVertexShaderRHIRef vertex;
        FPixelShaderRHIRef pixel;
        FGeometryShaderRHIRef geometry;
    };
}

#endif
