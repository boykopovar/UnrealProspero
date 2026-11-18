#include "UnrealProspero/SocketEndpoints.h"
#include "UnrealProspero/SocketAddress.h"
#include "prx/libkernel/include/Api.hpp"
#include <stdexcept>

namespace UnrealProspero::Network
{
    static int SendFlags(int flags)
    {
        if (flags & ~(MSG_DONTWAIT | MSG_NOSIGNAL)) throw std::invalid_argument("Unsupported socket send flag");
        return ((flags & MSG_DONTWAIT) ? 0x80 : 0) | ((flags & MSG_NOSIGNAL) ? 0x20000 : 0);
    }

    static int ReceiveFlags(int flags)
    {
        if (flags & ~(MSG_PEEK | MSG_WAITALL | MSG_DONTWAIT)) throw std::invalid_argument("Unsupported socket receive flag");
        return ((flags & MSG_PEEK) ? 0x2 : 0) | ((flags & MSG_WAITALL) ? 0x40 : 0) | ((flags & MSG_DONTWAIT) ? 0x80 : 0);
    }

    template<typename TQuery>
    static int QuerySocketName(TQuery query, int descriptor, sockaddr* address, socklen_t* length)
    {
        if (!address || !length) throw std::invalid_argument("Missing socket address output");
        SocketAddress native;
        const int result = query(descriptor, native.Data(), &native.Size());
        if (result == 0) native.Decode(*address, *length);
        return result;
    }

    int Bind(int descriptor, const sockaddr* address, socklen_t length)
    {
        if (!address) throw std::invalid_argument("Missing socket bind address");
        const auto native = SocketAddress::Encode(*address, length);
        return bind_nid_postfix(descriptor, native.Data(), native.Size());
    }

    int Connect(int descriptor, const sockaddr* address, socklen_t length)
    {
        if (!address) throw std::invalid_argument("Missing socket peer address");
        const auto native = SocketAddress::Encode(*address, length);
        return connect_nid_postfix(descriptor, native.Data(), native.Size());
    }

    int Accept(int descriptor, sockaddr* address, socklen_t* length)
    {
        if (!address) return accept_nid_postfix(descriptor, nullptr, nullptr);
        if (!length) throw std::invalid_argument("Missing accepted address length");
        SocketAddress native;
        const int client = accept_nid_postfix(descriptor, native.Data(), &native.Size());
        if (client < 0) return client;
        try
        {
            native.Decode(*address, *length);
        }
        catch (...)
        {
            close_nid_postfix(client);
            throw;
        }
        return client;
    }

    int GetSocketName(int descriptor, sockaddr* address, socklen_t* length)
    {
        return QuerySocketName(getsockname_nid_postfix, descriptor, address, length);
    }

    int GetPeerName(int descriptor, sockaddr* address, socklen_t* length)
    {
        return QuerySocketName(getpeername_nid_postfix, descriptor, address, length);
    }

    ssize_t Send(int descriptor, const void* data, std::size_t size, int flags)
    {
        return send_nid_postfix(descriptor, data, size, SendFlags(flags));
    }

    ssize_t SendTo(int descriptor, const void* data, std::size_t size, int flags, const sockaddr* address, socklen_t length)
    {
        if (!address) return Send(descriptor, data, size, flags);
        const auto native = SocketAddress::Encode(*address, length);
        return sendto_nid_postfix(descriptor, data, size, SendFlags(flags), native.Data(), native.Size());
    }

    ssize_t Receive(int descriptor, void* data, std::size_t size, int flags)
    {
        return recv_nid_postfix(descriptor, data, size, ReceiveFlags(flags));
    }

    ssize_t ReceiveFrom(int descriptor, void* data, std::size_t size, int flags, sockaddr* address, socklen_t* length)
    {
        if (!address) return Receive(descriptor, data, size, flags);
        if (!length) throw std::invalid_argument("Missing datagram address length");
        SocketAddress native;
        const auto result = recvfrom_nid_postfix(descriptor, data, size, ReceiveFlags(flags), native.Data(), &native.Size());
        if (result >= 0) native.Decode(*address, *length);
        return result;
    }
}
