#include "UnrealProspero/UnrealProsperoPlatformMisc.h"
#include "UnrealProspero/Process/Exit.h"
#include "CoreGlobals.h"

void FUnrealProsperoPlatformMisc::RequestExit(bool force, const TCHAR* callSite)
{
    if (force) UnrealProspero::Process::Exit::Force();
    RequestEngineExit(callSite != nullptr ? callSite : TEXT("UnrealProspero RequestExit"));
}

void FUnrealProsperoPlatformMisc::RequestExitWithStatus(bool force, uint8 status, const TCHAR* callSite)
{
    UnrealProspero::Process::Exit::SetStatus(status);
    RequestExit(force, callSite);
}
