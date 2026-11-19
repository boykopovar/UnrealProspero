#include "UnrealProspero/RHI/Device/DeviceLimits.h"
#include "RHIGlobals.h"
#include <stdexcept>

namespace UnrealProspero::RHI
{
    void ConfigureDeviceLimits()
    {
        const int32 dispatchLimit = GMaxComputeDispatchDimension;
        if (dispatchLimit <= 0) throw std::runtime_error("RHI compute dispatch limit must be positive");
        GRHISupportsAsyncTextureCreation = true;
        GSupportsVolumeTextureRendering = true;
        GMaxTextureDimensions = DeviceLimits::Texture2D;
        GMaxShadowDepthBufferSizeX = DeviceLimits::Texture2D;
        GMaxShadowDepthBufferSizeY = DeviceLimits::Texture2D;
        GMaxVolumeTextureDimensions = DeviceLimits::Texture3D;
        GRHIMaxDispatchThreadGroupsPerDimension = FIntVector(dispatchLimit, dispatchLimit, dispatchLimit);
    }
}
