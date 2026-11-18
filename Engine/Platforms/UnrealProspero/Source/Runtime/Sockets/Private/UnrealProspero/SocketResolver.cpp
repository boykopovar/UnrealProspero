#include "UnrealProspero/SocketResolver.h"
#include "UnrealProspero/SocketAddress.h"
#include "prx/libScePosixForWebKit/include/Api.hpp"
#include <cerrno>
#include <cstring>
#include <memory>
#include <stdexcept>

namespace UnrealProspero::Network
{
    namespace
    {
        constexpr int AddressFlags = AI_PASSIVE | AI_CANONNAME | AI_NUMERICHOST | AI_NUMERICSERV | AI_ALL | AI_ADDRCONFIG | AI_V4MAPPED;

        int ToGuestAddressFlags(int flags)
        {
            return ((flags & AI_PASSIVE) ? 1 : 0) | ((flags & AI_CANONNAME) ? 2 : 0) | ((flags & AI_NUMERICHOST) ? 4 : 0) | ((flags & AI_NUMERICSERV) ? 8 : 0) | ((flags & AI_ALL) ? 0x100 : 0) | ((flags & AI_ADDRCONFIG) ? 0x400 : 0) | ((flags & AI_V4MAPPED) ? 0x800 : 0);
        }

        int FromGuestResolverError(int error)
        {
            switch (error)
            {
            case 0: return 0;
            case 1: return EAI_ADDRFAMILY;
            case 2: return EAI_AGAIN;
            case 3: return EAI_BADFLAGS;
            case 4: return EAI_FAIL;
            case 5: return EAI_FAMILY;
            case 6: return EAI_MEMORY;
            case 7: return EAI_NODATA;
            case 8: return EAI_NONAME;
            case 9: return EAI_SERVICE;
            case 10: return EAI_SOCKTYPE;
            case 11: return EAI_SYSTEM;
            case 14: return EAI_OVERFLOW;
            default: throw std::runtime_error("Unknown PS5 resolver error");
            }
        }

        struct ResolvedAddress : addrinfo
        {
            sockaddr_storage storage{};
            std::unique_ptr<char[]> name;
        };
    }

    void FreeAddresses(addrinfo* addresses)
    {
        while (addresses)
        {
            addrinfo* next = addresses->ai_next;
            delete static_cast<ResolvedAddress*>(addresses);
            addresses = next;
        }
    }

    int ResolveAddress(const char* node, const char* service, const addrinfo* hints, addrinfo** output)
    {
        if (!output) throw std::invalid_argument("Missing resolver output");
        *output = nullptr;
        GuestResolver::AddressInfo guestHints{};
        if (hints)
        {
            if (hints->ai_flags & ~AddressFlags) return EAI_BADFLAGS;
            if (hints->ai_family != AF_UNSPEC && hints->ai_family != AF_INET && hints->ai_family != AF_INET6) return EAI_FAMILY;
            guestHints.flags = ToGuestAddressFlags(hints->ai_flags);
            guestHints.family = SocketAddress::ToGuestFamily(hints->ai_family);
            guestHints.socketType = hints->ai_socktype;
            guestHints.protocol = hints->ai_protocol;
        }
        GuestResolver::AddressInfo* guestOutput = nullptr;
        const int error = getaddrinfo_nid_postfix(node, service, hints ? &guestHints : nullptr, &guestOutput);
        std::unique_ptr<GuestResolver::AddressInfo, decltype(&freeaddrinfo_nid_postfix)> guestAddresses(guestOutput, freeaddrinfo_nid_postfix);
        if (error) return FromGuestResolverError(error);
        std::unique_ptr<addrinfo, decltype(&FreeAddresses)> addresses(nullptr, FreeAddresses);
        addrinfo* last = nullptr;
        for (const auto* guestAddress = guestAddresses.get(); guestAddress; guestAddress = guestAddress->next)
        {
            auto address = std::make_unique<ResolvedAddress>();
            address->ai_family = SocketAddress::FromGuestFamily(guestAddress->family);
            address->ai_flags = hints ? hints->ai_flags : 0;
            address->ai_socktype = guestAddress->socketType;
            address->ai_protocol = guestAddress->protocol;
            if (guestAddress->address)
            {
                SocketAddress nativeAddress;
                if (guestAddress->addressLength > nativeAddress.Size()) throw std::runtime_error("Oversized PS5 resolver address");
                std::memcpy(nativeAddress.Data(), guestAddress->address, guestAddress->addressLength);
                nativeAddress.Size() = guestAddress->addressLength;
                address->ai_addr = reinterpret_cast<sockaddr*>(&address->storage);
                address->ai_addrlen = sizeof(address->storage);
                nativeAddress.Decode(*address->ai_addr, address->ai_addrlen);
                if (address->ai_addr->sa_family != address->ai_family) throw std::runtime_error("Inconsistent PS5 resolver address family");
            }
            if (guestAddress->canonicalName)
            {
                const auto length = std::strlen(guestAddress->canonicalName) + 1;
                address->name = std::make_unique<char[]>(length);
                std::memcpy(address->name.get(), guestAddress->canonicalName, length);
                address->ai_canonname = address->name.get();
            }
            if (last) last->ai_next = address.get();
            else addresses.reset(address.get());
            last = address.release();
        }
        *output = addresses.release();
        return 0;
    }

    int ResolveName(const sockaddr* address, socklen_t length, char* host, socklen_t hostLength, char* service, socklen_t serviceLength, int flags)
    {
        if (!address) throw std::invalid_argument("Missing resolver address");
        if (flags & ~(NI_NOFQDN | NI_NUMERICHOST | NI_NAMEREQD | NI_NUMERICSERV | NI_DGRAM)) return EAI_BADFLAGS;
        const auto guestAddress = SocketAddress::Encode(*address, length);
        const int guestFlags = ((flags & NI_NOFQDN) ? 1 : 0) | ((flags & NI_NUMERICHOST) ? 2 : 0) | ((flags & NI_NAMEREQD) ? 4 : 0) | ((flags & NI_NUMERICSERV) ? 8 : 0) | ((flags & NI_DGRAM) ? 16 : 0);
        return FromGuestResolverError(getnameinfo_nid_postfix(guestAddress.Data(), guestAddress.Size(), host, hostLength, service, serviceLength, guestFlags));
    }
}
