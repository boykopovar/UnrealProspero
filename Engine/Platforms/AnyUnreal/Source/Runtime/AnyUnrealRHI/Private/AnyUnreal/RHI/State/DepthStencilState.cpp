#include "AnyUnreal/RHI/State/DepthStencilState.h"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    namespace
    {
        void checkComparison(ECompareFunction function)
        {
            switch (function)
            {
            case CF_Less:
            case CF_LessEqual:
            case CF_Greater:
            case CF_GreaterEqual:
            case CF_Equal:
            case CF_NotEqual:
            case CF_Never:
            case CF_Always: break;
            default: throw std::invalid_argument("Unknown depth or stencil comparison function");
            }
        }

        void checkOperation(EStencilOp operation)
        {
            switch (operation)
            {
            case SO_Keep:
            case SO_Zero:
            case SO_Replace:
            case SO_SaturatedIncrement:
            case SO_SaturatedDecrement:
            case SO_Invert:
            case SO_Increment:
            case SO_Decrement: break;
            default: throw std::invalid_argument("Unknown stencil operation");
            }
        }
    }

    DepthStencilState::DepthStencilState(const FDepthStencilStateInitializerRHI& initializer) : description(initializer)
    {
        checkComparison(description.DepthTest);
        checkComparison(description.FrontFaceStencilTest);
        checkComparison(description.BackFaceStencilTest);
        checkOperation(description.FrontFaceStencilFailStencilOp);
        checkOperation(description.FrontFaceDepthFailStencilOp);
        checkOperation(description.FrontFacePassStencilOp);
        checkOperation(description.BackFaceStencilFailStencilOp);
        checkOperation(description.BackFaceDepthFailStencilOp);
        checkOperation(description.BackFacePassStencilOp);
    }

    bool DepthStencilState::GetInitializer(FDepthStencilStateInitializerRHI& result)
    {
        result = description;
        return true;
    }

    const FDepthStencilStateInitializerRHI& DepthStencilState::GetDescription() const { return description; }
}
