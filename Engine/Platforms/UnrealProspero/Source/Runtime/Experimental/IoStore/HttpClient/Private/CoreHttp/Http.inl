#ifndef UNREAL_PROSPERO_IO_STORE_HTTP_INL
#define UNREAL_PROSPERO_IO_STORE_HTTP_INL

#include "prx/libkernel/include/Api.hpp"
#include "prx/libScePosixForWebKit/include/Api.hpp"
#include <cerrno>
#include <memory>
#include <stdexcept>

#define IAS_HTTP_USE_POLL
#define IAS_HTTP_HAS_NONBLOCK_IMPL
#undef IAS_HTTP_SOCKET_FORWARD
#undef EAGAIN
#undef EWOULDBLOCK
#undef EINPROGRESS
#undef ENOTCONN
#define EAGAIN 35
#define EWOULDBLOCK 35
#define EINPROGRESS 36
#define ENOTCONN 57

namespace UE::IoStore::HTTP
{
    using SocketType = int32;
    using MsgFlagType = int32;
    using socklen_t = uint32;
    using PollType = UPTRINT;

    struct sockaddr
    {
        uint8 length;
        uint8 family;
        uint8 data[14];
    };

    struct sockaddr_in
    {
        uint8 length;
        uint8 sin_family;
        uint16 sin_port;
        uint32 sin_addr;
        uint8 reserved[8];
    };

    struct pollfd
    {
        int32 fd;
        int16 events;
        int16 revents;
    };

    static_assert(sizeof(sockaddr_in) == sizeof(sockaddr));
    static_assert(sizeof(sockaddr_in) == 16);

    constexpr int32 AF_INET = 2;
    constexpr int32 SOCK_STREAM = 1;
    constexpr int32 IPPROTO_TCP = 6;
    constexpr int32 TCP_NODELAY = 1;
    constexpr int32 SOL_SOCKET = 0xffff;
    constexpr int32 SO_SNDBUF = 0x1001;
    constexpr int32 SO_RCVBUF = 0x1002;
    constexpr int32 SO_ERROR = 0x1007;
    constexpr int32 SHUT_RDWR = 2;
    constexpr int32 MSG_NOSIGNAL = 0x20000;
    constexpr int16 POLLIN = 0x1;
    constexpr int16 POLLOUT = 0x4;
    constexpr int16 POLLERR = 0x8;
    constexpr int16 POLLHUP = 0x10;
    constexpr int16 POLLNVAL = 0x20;

    static uint16 htons(uint16 value) { return ByteSwap(value); }
    static uint32 htonl(uint32 value) { return ByteSwap(value); }

    static SocketType Socket_Socket(int32 family, int32 type, int32 protocol) { return socket_nid_postfix(family, type, protocol); }
    static int32 Socket_Close(SocketType socket) { return close_nid_postfix(socket); }
    static int32 Socket_Connect(SocketType socket, const void* address, uint32 length) { return connect_nid_postfix(socket, address, length); }
    static int32 Socket_Shutdown(SocketType socket, int32 how) { return shutdown_nid_postfix(socket, how); }
    static int32 Socket_GetSockOpt(SocketType socket, int32 level, int32 option, void* value, socklen_t* length) { return getsockopt_nid_postfix(socket, level, option, value, length); }
    static int32 Socket_SetSockOpt(SocketType socket, int32 level, int32 option, const void* value, socklen_t length) { return setsockopt_nid_postfix(socket, level, option, value, length); }
    static int32 Socket_Send(SocketType socket, const void* data, uint32 size, MsgFlagType flags) { return static_cast<int32>(send_nid_postfix(socket, data, size, flags)); }
    static int32 Socket_Recv(SocketType socket, void* data, uint32 size, MsgFlagType flags) { return static_cast<int32>(recv_nid_postfix(socket, data, size, flags)); }

    static bool SetNonBlockingSocket(SocketType socket, bool nonBlocking)
    {
        constexpr int32 getStatus = 3;
        constexpr int32 setStatus = 4;
        constexpr int32 nonBlockingFlag = 0x4;
        const int32 flags = fcntl_nid_postfix(socket, getStatus);
        if (flags < 0) return false;
        const int32 updated = nonBlocking ? flags | nonBlockingFlag : flags & ~nonBlockingFlag;
        return updated == flags || fcntl_nid_postfix(socket, setStatus, updated) == 0;
    }

    static PollType Poll_CreateContext() { return 0; }
    static void Poll_DestroyContext(PollType) {}
    static bool Poll_Register(PollType, SocketType) { return true; }

    template<typename TPoll>
    static int32 Poll_Wait(PollType, TPoll* polls, int32 count, int32 timeout)
    {
        check(count >= 0);
        TArray<GuestPollDescriptor, TInlineAllocator<64>> descriptors;
        descriptors.Reserve(count);
        for (int32 index = 0; index < count; ++index) descriptors.Add({polls[index].fd, polls[index].events, 0});
        const int32 result = poll_nid_postfix(descriptors.GetData(), static_cast<uint32>(count), timeout);
        if (result >= 0)
            for (int32 index = 0; index < count; ++index) polls[index].revents = descriptors[index].revents;
        return result;
    }

    struct addrinfo
    {
        int32 ai_flags{};
        int32 ai_family{};
        int32 ai_socktype{};
        int32 ai_protocol{};
        socklen_t ai_addrlen{};
        char* ai_canonname{};
        sockaddr* ai_addr{};
        addrinfo* ai_next{};
        GuestResolver::AddressInfo* owner{};
    };

    static void Socket_FreeAddrInfo(addrinfo* addresses)
    {
        if (addresses && addresses->owner) freeaddrinfo_nid_postfix(addresses->owner);
        while (addresses)
        {
            auto* next = addresses->ai_next;
            delete addresses;
            addresses = next;
        }
    }

    static int32 Socket_GetAddrInfo(const char* host, const char* service, const addrinfo* hints, addrinfo** output)
    {
        if (!output) throw std::invalid_argument("Missing IoStore resolver output");
        *output = nullptr;
        GuestResolver::AddressInfo nativeHints{};
        if (hints)
        {
            nativeHints.flags = hints->ai_flags;
            nativeHints.family = hints->ai_family;
            nativeHints.socketType = hints->ai_socktype;
            nativeHints.protocol = hints->ai_protocol;
        }
        GuestResolver::AddressInfo* nativeOutput = nullptr;
        const int32 result = getaddrinfo_nid_postfix(host, service, hints ? &nativeHints : nullptr, &nativeOutput);
        std::unique_ptr<GuestResolver::AddressInfo, decltype(&freeaddrinfo_nid_postfix)> nativeAddresses(nativeOutput, freeaddrinfo_nid_postfix);
        if (result != 0) return result;
        std::unique_ptr<addrinfo, decltype(&Socket_FreeAddrInfo)> addresses(nullptr, Socket_FreeAddrInfo);
        addrinfo* last = nullptr;
        for (auto* native = nativeOutput; native; native = native->next)
        {
            auto address = std::make_unique<addrinfo>();
            address->ai_flags = native->flags;
            address->ai_family = native->family;
            address->ai_socktype = native->socketType;
            address->ai_protocol = native->protocol;
            address->ai_addrlen = native->addressLength;
            address->ai_canonname = native->canonicalName;
            address->ai_addr = static_cast<sockaddr*>(native->address);
            if (!address->ai_addr || address->ai_addrlen < sizeof(sockaddr) || address->ai_addr->family != address->ai_family) throw std::runtime_error("Invalid IoStore resolver address");
            if (last) last->ai_next = address.get();
            else addresses.reset(address.get());
            last = address.release();
        }
        if (addresses) addresses->owner = nativeAddresses.release();
        *output = addresses.release();
        return result;
    }
}

#endif
