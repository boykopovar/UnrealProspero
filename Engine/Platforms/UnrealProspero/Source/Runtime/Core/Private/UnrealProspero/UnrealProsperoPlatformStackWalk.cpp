#include "UnrealProspero/UnrealProsperoPlatformStackWalk.h"
#include <stdexcept>

bool FUnrealProsperoPlatformStackWalk::InitStackWalking()
{
    throw std::runtime_error("UnrealProspero stack unwinding and symbol resolution are not implemented");
}

bool FUnrealProsperoPlatformStackWalk::InitStackWalkingForProcess(const FProcHandle&)
{
    throw std::runtime_error("UnrealProspero stack unwinding and symbol resolution are not implemented");
}

void FUnrealProsperoPlatformStackWalk::ProgramCounterToSymbolInfo(uint64, FProgramCounterSymbolInfo&)
{
    throw std::runtime_error("UnrealProspero stack unwinding and symbol resolution are not implemented");
}

void FUnrealProsperoPlatformStackWalk::ProgramCounterToSymbolInfoEx(uint64, FProgramCounterSymbolInfoEx&)
{
    throw std::runtime_error("UnrealProspero stack unwinding and symbol resolution are not implemented");
}

uint32 FUnrealProsperoPlatformStackWalk::CaptureStackBackTrace(uint64*, uint32, void*)
{
    throw std::runtime_error("UnrealProspero stack unwinding and symbol resolution are not implemented");
}

uint32 FUnrealProsperoPlatformStackWalk::CaptureThreadStackBackTrace(uint64, uint64*, uint32, void*)
{
    throw std::runtime_error("UnrealProspero stack unwinding and symbol resolution are not implemented");
}

void FUnrealProsperoPlatformStackWalk::ThreadStackWalkAndDump(ANSICHAR*, SIZE_T, int32, uint32)
{
    throw std::runtime_error("UnrealProspero stack unwinding and symbol resolution are not implemented");
}

int32 FUnrealProsperoPlatformStackWalk::GetProcessModuleCount()
{
    throw std::runtime_error("UnrealProspero stack unwinding and symbol resolution are not implemented");
}

int32 FUnrealProsperoPlatformStackWalk::GetProcessModuleSignatures(FStackWalkModuleInfo*, int32)
{
    throw std::runtime_error("UnrealProspero stack unwinding and symbol resolution are not implemented");
}
