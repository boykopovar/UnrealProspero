#ifndef UNREAL_PROSPERO_IO_STORE_HTTP_INL
#define UNREAL_PROSPERO_IO_STORE_HTTP_INL

#include "prx/libkernel/include/Api.hpp"
#include "prx/libScePosixForWebKit/include/Api.hpp"
#include <cerrno>

#define IAS_HTTP_USE_POLL
#define IAS_HTTP_HAS_NONBLOCK_IMPL

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
    constexpr int32 Prospero_EWOULDBLOCK = 35;
    constexpr int32 Prospero_EINPROGRESS = 36;
    constexpr int32 Prospero_ENOTCONN = 57;

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

    static int32 ResolveProsperoHost(const char* host, TArrayView<uint32> addresses)
    {
        GuestResolver::AddressInfo hints{};
        hints.family = AF_INET;
        hints.socketType = SOCK_STREAM;
        hints.protocol = IPPROTO_TCP;
        GuestResolver::AddressInfo* first = nullptr;
        if (getaddrinfo_nid_postfix(host, nullptr, &hints, &first) != 0) return -1;
        ON_SCOPE_EXIT { if (first) freeaddrinfo_nid_postfix(first); };
        int32 count = 0;
        for (auto* current = first; current && count < addresses.Num(); current = current->next)
        {
            if (current->family != AF_INET || current->addressLength != sizeof(sockaddr_in) || !current->address) continue;
            sockaddr_in address{};
            FMemory::Memcpy(&address, current->address, sizeof(address));
            if (address.sin_family == AF_INET && address.sin_addr != 0) addresses[count++] = ByteSwap(address.sin_addr);
        }
        return count;
    }
}

#endif
