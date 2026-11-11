#include "UnrealProspero/UnrealProsperoPlatformProcess.h"
#include "UnrealProspero/UnrealProsperoRunnableThread.h"

FRunnableThread* FUnrealProsperoPlatformProcess::CreateRunnableThread()
{
    return new FUnrealProsperoRunnableThread();
}
