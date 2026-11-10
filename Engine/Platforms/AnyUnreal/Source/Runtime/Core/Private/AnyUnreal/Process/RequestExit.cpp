#include "AnyUnreal/AnyUnrealPlatformMisc.h"
#include "AnyUnreal/Process/Exit.h"
#include "CoreGlobals.h"

void FAnyUnrealPlatformMisc::RequestExit(bool force, const TCHAR* callSite)
{
    if (force) AnyUnreal::Process::Exit::Force();
    RequestEngineExit(callSite != nullptr ? callSite : TEXT("AnyUnreal RequestExit"));
}

void FAnyUnrealPlatformMisc::RequestExitWithStatus(bool force, uint8 status, const TCHAR* callSite)
{
    AnyUnreal::Process::Exit::SetStatus(status);
    RequestExit(force, callSite);
}
