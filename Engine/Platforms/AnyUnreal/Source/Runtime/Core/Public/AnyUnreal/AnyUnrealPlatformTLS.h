#ifndef ANYUNREAL_PLATFORM_TLS_H
#define ANYUNREAL_PLATFORM_TLS_H

#include "GenericPlatform/GenericPlatformTLS.h"

struct FAnyUnrealPlatformTLS : FGenericPlatformTLS
{
    static CORE_API uint32 GetCurrentThreadId();
    static CORE_API uint32 AllocTlsSlot();
    static CORE_API void SetTlsValue(uint32 slotIndex, void* value);
    static CORE_API void* GetTlsValue(uint32 slotIndex);
    static CORE_API void FreeTlsSlot(uint32 slotIndex);
};

using FPlatformTLS = FAnyUnrealPlatformTLS;

#endif
