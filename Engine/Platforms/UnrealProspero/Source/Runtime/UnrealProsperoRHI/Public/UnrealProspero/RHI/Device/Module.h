#ifndef UNREALPROSPERO_RHI_DEVICE_MODULE_H
#define UNREALPROSPERO_RHI_DEVICE_MODULE_H

#include <DynamicRHI.h>

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API Module final : public IDynamicRHIModule
    {
    public:
        bool IsSupported() override;
        bool IsSupported(ERHIFeatureLevel::Type requestedFeatureLevel) override;
        FDynamicRHI* CreateRHI(ERHIFeatureLevel::Type requestedFeatureLevel) override;
    };
}

#endif
