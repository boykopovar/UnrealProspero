#include "HAL/Platform.h"

#if UNREALPROSPERO_TARGET_PS5

#include "BSDSockets/SocketSubsystemBSD.h"
#include "SocketSubsystemModule.h"

namespace
{
    class FSocketSubsystemUnrealProspero final : public FSocketSubsystemBSD
    {
    public:
        bool Init(FString&) override
        {
            return true;
        }

        void Shutdown() override
        {
        }

        bool HasNetworkDevice() override
        {
            auto address = CreateInternetAddr();
            return GetLocalHostAddrViaConnect(address);
        }
    };

    FSocketSubsystemUnrealProspero* Subsystem = nullptr;
}

FName CreateSocketSubsystem(FSocketSubsystemModule& module)
{
    const FName name(TEXT("UNREALPROSPERO"));
    auto subsystem = MakeUnique<FSocketSubsystemUnrealProspero>();
    FString error;
    if (!subsystem->Init(error)) return NAME_None;
    module.RegisterSocketSubsystem(name, subsystem.Get());
    Subsystem = subsystem.Release();
    return name;
}

void DestroySocketSubsystem(FSocketSubsystemModule& module)
{
    if (!Subsystem) return;
    module.UnregisterSocketSubsystem(FName(TEXT("UNREALPROSPERO")));
    Subsystem->Shutdown();
    delete Subsystem;
    Subsystem = nullptr;
}

#endif
