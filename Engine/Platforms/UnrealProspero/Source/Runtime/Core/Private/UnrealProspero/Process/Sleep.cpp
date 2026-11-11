#include "UnrealProspero/UnrealProsperoPlatformProcess.h"
#include "Stats/ThreadIdleStats.h"

void FUnrealProsperoPlatformProcess::Sleep(float seconds)
{
    UE::Stats::FThreadIdleStats::FScopeIdle scope;
    SleepNoStats(seconds);
}
