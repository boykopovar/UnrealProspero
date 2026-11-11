#include "CoreMinimal.h"
#include "HAL/PlatformMath.h"
#include "Math/Transform.h"

static_assert(ENABLE_VECTORIZED_TRANSFORM);

bool CheckMath()
{
    FTransform3f transform(FQuat4f(0.0f, 0.0f, 0.0f, 1.0f), FVector3f(10.0f, 20.0f, 30.0f), FVector3f(2.0f, 3.0f, 4.0f));
    FVector3f point = transform.TransformPosition(FVector3f(1.0f, 2.0f, 3.0f));
    if (point != FVector3f(12.0f, 26.0f, 42.0f)) return false;
    if (!transform.InverseTransformPosition(point).Equals(FVector3f(1.0f, 2.0f, 3.0f), 0.0001f)) return false;
    if (FPlatformMath::CountLeadingZeros(0) != 32 || FPlatformMath::CountTrailingZeros(0) != 32) return false;
    if (FPlatformMath::CountLeadingZeros64(0) != 64 || FPlatformMath::CountTrailingZeros64(0) != 64) return false;
    for (uint32 bit = 0; bit < 64; ++bit)
    {
        uint64 value = uint64{1} << bit;
        if (FPlatformMath::CountLeadingZeros64(value) != 63 - bit || FPlatformMath::CountTrailingZeros64(value) != bit) return false;
        if (FPlatformMath::FloorLog2_64(value) != bit || FPlatformMath::CeilLogTwo64(value) != bit) return false;
    }
    volatile double input = 6.25;
    if (FPlatformMath::Sqrt(input) != 2.5 || FPlatformMath::FloorToInt32(-input) != -7) return false;
    if (FPlatformMath::RoundToInt32(input) != 6 || FPlatformMath::RoundToInt32(-input) != -6) return false;
    uint16 encoded = 0;
    FPlatformMath::StoreHalf(&encoded, 1.5f);
    return encoded == 0x3e00 && FPlatformMath::LoadHalf(&encoded) == 1.5f;
}
