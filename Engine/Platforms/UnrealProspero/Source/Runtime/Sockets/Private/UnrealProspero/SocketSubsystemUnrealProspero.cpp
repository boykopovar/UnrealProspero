#include "HAL/Platform.h"

#if UNREALPROSPERO_TARGET_PS5

#include "BSDSockets/SocketSubsystemBSD.h"
#include "SocketSubsystemModule.h"
#include <cerrno>

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

        ESocketErrors TranslateErrorCode(int32 code) override
        {
            switch (code)
            {
            case -1: return GetLastErrorCode();
            case 0: return SE_NO_ERROR;
            case 4: return SE_EINTR;
            case 9: return SE_EBADF;
            case 13: return SE_EACCES;
            case 14: return SE_EFAULT;
            case 19: return SE_NODEV;
            case 22: return SE_EINVAL;
            case 24: return SE_EMFILE;
            case 32: return SE_ECONNRESET;
            case 35: return SE_EWOULDBLOCK;
            case 36: return SE_EINPROGRESS;
            case 37: return SE_EALREADY;
            case 38: return SE_ENOTSOCK;
            case 39: return SE_EDESTADDRREQ;
            case 40: return SE_EMSGSIZE;
            case 41: return SE_EPROTOTYPE;
            case 42: return SE_ENOPROTOOPT;
            case 43: return SE_EPROTONOSUPPORT;
            case 44: return SE_ESOCKTNOSUPPORT;
            case 45: return SE_EOPNOTSUPP;
            case 46: return SE_EPFNOSUPPORT;
            case 47: return SE_EAFNOSUPPORT;
            case 48: return SE_EADDRINUSE;
            case 49: return SE_EADDRNOTAVAIL;
            case 50: return SE_ENETDOWN;
            case 51: return SE_ENETUNREACH;
            case 52: return SE_ENETRESET;
            case 53: return SE_ECONNABORTED;
            case 54: return SE_ECONNRESET;
            case 55: return SE_ENOBUFS;
            case 56: return SE_EISCONN;
            case 57: return SE_ENOTCONN;
            case 58: return SE_ESHUTDOWN;
            case 59: return SE_ETOOMANYREFS;
            case 60: return SE_ETIMEDOUT;
            case 61: return SE_ECONNREFUSED;
            case 62: return SE_ELOOP;
            case 63: return SE_ENAMETOOLONG;
            case 64: return SE_EHOSTDOWN;
            case 65: return SE_EHOSTUNREACH;
            case 66: return SE_ENOTEMPTY;
            case 67: return SE_EPROCLIM;
            case 68: return SE_EUSERS;
            case 69: return SE_EDQUOT;
            case 70: return SE_ESTALE;
            case 71: return SE_EREMOTE;
            default: return SE_SYSTEM;
            }
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
