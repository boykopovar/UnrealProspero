#include "UnrealProspero/SocketUnrealProspero.h"
#include "UnrealProspero/SocketOptions.h"
#include <cstdint>
#include <netinet/in.h>
#include <stdexcept>

bool FSocketUnrealProspero::SetMulticastLoopback(bool loopback)
{
    if (GetProtocol() == FNetworkProtocolTypes::IPv6)
    {
        const std::uint32_t enabled = loopback ? 1 : 0;
        return UnrealProspero::Network::SetSocketOption(GetNativeSocket(), IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &enabled, sizeof(enabled)) == 0;
    }
    if (GetProtocol() == FNetworkProtocolTypes::IPv4)
    {
        const std::uint8_t enabled = loopback ? 1 : 0;
        return UnrealProspero::Network::SetSocketOption(GetNativeSocket(), IPPROTO_IP, IP_MULTICAST_LOOP, &enabled, sizeof(enabled)) == 0;
    }
    throw std::invalid_argument("Unsupported multicast socket protocol");
}
