#include "HAL/PlatformMath.h"

bool CheckMath()
{
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
