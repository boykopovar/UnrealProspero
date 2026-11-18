#include "UnrealProspero/SocketAddress.h"
#include <algorithm>
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>

namespace UnrealProspero::Network
{
    int SocketAddress::ToGuestFamily(int family)
    {
        switch (family)
        {
        case AF_UNSPEC: return 0;
        case AF_INET: return Ipv4;
        case AF_INET6: return Ipv6;
        default: throw std::invalid_argument("Unsupported socket address family");
        }
    }

    int SocketAddress::FromGuestFamily(int family)
    {
        switch (family)
        {
        case 0: return AF_UNSPEC;
        case Ipv4: return AF_INET;
        case Ipv6: return AF_INET6;
        default: throw std::invalid_argument("Unsupported PS5 socket address family");
        }
    }

    SocketAddress SocketAddress::Encode(const sockaddr& address, socklen_t size)
    {
        static_assert(sizeof(sockaddr_in) == 16 && offsetof(sockaddr_in, sin_port) == 2);
        static_assert(sizeof(sockaddr_in6) == 28 && offsetof(sockaddr_in6, sin6_port) == 2);
        if (size < sizeof(address.sa_family)) throw std::invalid_argument("Socket address lacks a family");
        SocketAddress result;
        const auto* source = reinterpret_cast<const std::byte*>(&address);
        sa_family_t sourceFamily;
        std::memcpy(&sourceFamily, source, sizeof(sourceFamily));
        const int family = ToGuestFamily(sourceFamily);
        if (family == Ipv4) result.length = sizeof(sockaddr_in);
        else if (family == Ipv6) result.length = sizeof(sockaddr_in6);
        else throw std::invalid_argument("Socket address requires a concrete family");
        if (size < result.length) throw std::invalid_argument("Truncated socket address");
        std::memcpy(result.bytes.data() + 2, source + 2, result.length - 2);
        result.bytes[0] = static_cast<std::byte>(result.length);
        result.bytes[1] = static_cast<std::byte>(family);
        return result;
    }

    void SocketAddress::Decode(sockaddr& address, socklen_t& size) const
    {
        if (length < 2 || length > bytes.size()) throw std::invalid_argument("Invalid PS5 socket address length");
        const int family = FromGuestFamily(std::to_integer<int>(bytes[1]));
        const socklen_t expected = family == AF_INET ? sizeof(sockaddr_in) : family == AF_INET6 ? sizeof(sockaddr_in6) : 0;
        if (!expected || length != expected) throw std::invalid_argument("Inconsistent PS5 socket address");
        auto decoded = bytes;
        const auto linuxFamily = static_cast<sa_family_t>(family);
        std::memcpy(decoded.data(), &linuxFamily, sizeof(linuxFamily));
        std::memcpy(&address, decoded.data(), std::min(size, expected));
        size = expected;
    }

    const void* SocketAddress::Data() const { return bytes.data(); }
    void* SocketAddress::Data() { return bytes.data(); }
    std::uint32_t SocketAddress::Size() const { return length; }
    std::uint32_t& SocketAddress::Size() { return length; }
}
