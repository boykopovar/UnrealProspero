#include "UnrealProspero/UnrealProsperoPlatformTLS.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include <bit>
#include <stdexcept>

namespace
{
    PthreadKey getKey(uint32 slotIndex)
    {
        if (!FPlatformTLS::IsValidTlsSlot(slotIndex)) throw std::invalid_argument("Invalid TLS slot");
        return std::bit_cast<PthreadKey>(slotIndex);
    }
}

uint32 FUnrealProsperoPlatformTLS::GetCurrentThreadId()
{
    return std::bit_cast<uint32>(scePthreadGetthreadid());
}

uint32 FUnrealProsperoPlatformTLS::AllocTlsSlot()
{
    PthreadKey key;
    UnrealProspero::Kernel::CheckResult(scePthreadKeyCreate(&key, nullptr), __func__);
    return std::bit_cast<uint32>(key);
}

void FUnrealProsperoPlatformTLS::SetTlsValue(uint32 slotIndex, void* value)
{
    UnrealProspero::Kernel::CheckResult(scePthreadSetspecific(getKey(slotIndex), value), __func__);
}

void* FUnrealProsperoPlatformTLS::GetTlsValue(uint32 slotIndex)
{
    return scePthreadGetspecific(getKey(slotIndex));
}

void FUnrealProsperoPlatformTLS::FreeTlsSlot(uint32 slotIndex)
{
    UnrealProspero::Kernel::CheckResult(scePthreadKeyDelete(getKey(slotIndex)), __func__);
}
