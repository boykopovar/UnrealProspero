#include "HAL/PlatformTLS.h"
#include "prx/libkernel/include/Api.hpp"
#include <cstddef>
#include <stdexcept>

std::size_t AnyUnrealRuntimeProbe(std::size_t count);
bool CheckMutexes();
bool CheckEvents();
bool CheckAtomics();
bool CheckMath();
bool CheckMisc();
bool CheckStrings();
bool CheckClocks();
bool CheckTime();
bool CheckMemory();

namespace
{
    struct ThreadState
    {
        uint32 slot;
        uint32 parentId;
        bool passed = false;
    };

    void* APS5_VABI checkThread(void* argument)
    {
        auto& state = *static_cast<ThreadState*>(argument);
        if (FPlatformTLS::GetCurrentThreadId() == state.parentId) return nullptr;
        if (FPlatformTLS::GetTlsValue(state.slot) != nullptr) return nullptr;
        FPlatformTLS::SetTlsValue(state.slot, &state);
        state.passed = FPlatformTLS::GetTlsValue(state.slot) == &state;
        return nullptr;
    }
}

bool CheckThreads();
bool CheckProcess();
bool CheckFiles();
bool CheckFileSystem();
bool CheckFileMetadata();
bool CheckDirectories();

bool CheckFileLocks();
int RunFileLockProbe(int count, const char** arguments);

int main()
{
    const int count = getargc_nid_postfix();
    const char** arguments = getargv_nid_postfix();
    if (count != 1) return RunFileLockProbe(count, arguments);
    if (!CheckFileLocks()) return 24;
    if (AnyUnrealRuntimeProbe(3) != 27) return 1;
    if (!CheckMutexes()) return 10;
    if (!CheckEvents()) return 11;
    if (!CheckAtomics()) return 12;
    if (!CheckMath()) return 13;
    if (!CheckMisc()) return 14;
    if (!CheckStrings()) return 15;
    if (!CheckClocks()) return 16;
    if (!CheckTime()) return 18;
    if (!CheckThreads()) return 25;
    if (!CheckProcess()) return 19;
    if (!CheckFiles()) return 20;
    if (!CheckFileSystem()) return 21;
    if (!CheckFileMetadata()) return 22;
    if (!CheckDirectories()) return 23;
    if (!CheckMemory()) return 17;
    uint32 slot = FPlatformTLS::AllocTlsSlot();
    if (!FPlatformTLS::IsValidTlsSlot(slot)) return 2;
    if (FPlatformTLS::GetTlsValue(slot) != nullptr) return 3;
    int value = 42;
    FPlatformTLS::SetTlsValue(slot, &value);
    ThreadState state{slot, FPlatformTLS::GetCurrentThreadId()};
    Pthread thread;
    if (scePthreadCreate(&thread, nullptr, checkThread, &state, "AnyUnrealTLS") != 0) return 4;
    if (scePthreadJoin(thread, nullptr) != 0) return 5;
    if (!state.passed || FPlatformTLS::GetTlsValue(slot) != &value) return 6;
    FPlatformTLS::SetTlsValue(slot, nullptr);
    if (FPlatformTLS::GetTlsValue(slot) != nullptr) return 7;
    FPlatformTLS::FreeTlsSlot(slot);
    uint32 replacement = FPlatformTLS::AllocTlsSlot();
    if (FPlatformTLS::GetTlsValue(replacement) != nullptr) return 8;
    FPlatformTLS::FreeTlsSlot(replacement);
    try
    {
        FPlatformTLS::GetTlsValue(FPlatformTLS::InvalidTlsSlot);
        return 9;
    }
    catch (const std::invalid_argument&)
    {
    }
    return 0;
}
