#ifndef UNREAL_PROSPERO_SOCKET_RESOLVER_H
#define UNREAL_PROSPERO_SOCKET_RESOLVER_H

#include <cstddef>
#include <netdb.h>

namespace UnrealProspero::Network
{
    int ReadHostName(char* name, std::size_t length);
    int ResolveAddress(const char* node, const char* service, const addrinfo* hints, addrinfo** output);
    void FreeAddresses(addrinfo* addresses);
    int ResolveName(const sockaddr* address, socklen_t length, char* host, socklen_t hostLength, char* service, socklen_t serviceLength, int flags);
}

#endif
