#include "AnyUnreal/AnyUnrealPlatformTLS.h"
#include "AnyUnreal/Kernel/Result.h"
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

uint32 FAnyUnrealPlatformTLS::GetCurrentThreadId()
{
    return std::bit_cast<uint32>(scePthreadGetthreadid());
}

uint32 FAnyUnrealPlatformTLS::AllocTlsSlot()
{
    PthreadKey key;
    AnyUnreal::Kernel::CheckResult(scePthreadKeyCreate(&key, nullptr), __func__);
    return std::bit_cast<uint32>(key);
}

void FAnyUnrealPlatformTLS::SetTlsValue(uint32 slotIndex, void* value)
{
    AnyUnreal::Kernel::CheckResult(scePthreadSetspecific(getKey(slotIndex), value), __func__);
}

void* FAnyUnrealPlatformTLS::GetTlsValue(uint32 slotIndex)
{
    return scePthreadGetspecific(getKey(slotIndex));
}

void FAnyUnrealPlatformTLS::FreeTlsSlot(uint32 slotIndex)
{
    AnyUnreal::Kernel::CheckResult(scePthreadKeyDelete(getKey(slotIndex)), __func__);
}
