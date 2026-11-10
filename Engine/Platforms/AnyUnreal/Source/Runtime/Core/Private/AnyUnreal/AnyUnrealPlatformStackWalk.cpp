#include "AnyUnreal/AnyUnrealPlatformStackWalk.h"
#include <stdexcept>

bool FAnyUnrealPlatformStackWalk::InitStackWalking()
{
    throw std::runtime_error("AnyUnreal stack unwinding and symbol resolution are not implemented");
}

bool FAnyUnrealPlatformStackWalk::InitStackWalkingForProcess(const FProcHandle&)
{
    throw std::runtime_error("AnyUnreal stack unwinding and symbol resolution are not implemented");
}

void FAnyUnrealPlatformStackWalk::ProgramCounterToSymbolInfo(uint64, FProgramCounterSymbolInfo&)
{
    throw std::runtime_error("AnyUnreal stack unwinding and symbol resolution are not implemented");
}

void FAnyUnrealPlatformStackWalk::ProgramCounterToSymbolInfoEx(uint64, FProgramCounterSymbolInfoEx&)
{
    throw std::runtime_error("AnyUnreal stack unwinding and symbol resolution are not implemented");
}

uint32 FAnyUnrealPlatformStackWalk::CaptureStackBackTrace(uint64*, uint32, void*)
{
    throw std::runtime_error("AnyUnreal stack unwinding and symbol resolution are not implemented");
}

uint32 FAnyUnrealPlatformStackWalk::CaptureThreadStackBackTrace(uint64, uint64*, uint32, void*)
{
    throw std::runtime_error("AnyUnreal stack unwinding and symbol resolution are not implemented");
}

void FAnyUnrealPlatformStackWalk::ThreadStackWalkAndDump(ANSICHAR*, SIZE_T, int32, uint32)
{
    throw std::runtime_error("AnyUnreal stack unwinding and symbol resolution are not implemented");
}

int32 FAnyUnrealPlatformStackWalk::GetProcessModuleCount()
{
    throw std::runtime_error("AnyUnreal stack unwinding and symbol resolution are not implemented");
}

int32 FAnyUnrealPlatformStackWalk::GetProcessModuleSignatures(FStackWalkModuleInfo*, int32)
{
    throw std::runtime_error("AnyUnreal stack unwinding and symbol resolution are not implemented");
}
