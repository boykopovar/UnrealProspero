#include "AnyUnreal/AnyUnrealPlatformProcess.h"
#include "Stats/ThreadIdleStats.h"

void FAnyUnrealPlatformProcess::Sleep(float seconds)
{
    UE::Stats::FThreadIdleStats::FScopeIdle scope;
    SleepNoStats(seconds);
}
