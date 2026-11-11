#ifndef ANYUNREAL_RHI_DEVICE_MODULE_H
#define ANYUNREAL_RHI_DEVICE_MODULE_H

#include <DynamicRHI.h>

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API Module final : public IDynamicRHIModule
    {
    public:
        bool IsSupported() override;
        bool IsSupported(ERHIFeatureLevel::Type requestedFeatureLevel) override;
        FDynamicRHI* CreateRHI(ERHIFeatureLevel::Type requestedFeatureLevel) override;
    };
}

#endif
