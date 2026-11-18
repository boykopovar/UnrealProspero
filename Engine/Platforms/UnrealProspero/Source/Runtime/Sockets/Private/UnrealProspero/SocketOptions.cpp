#include "UnrealProspero/SocketOptions.h"
#include "prx/libkernel/include/Api.hpp"
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdexcept>
#include <sys/ioctl.h>

namespace UnrealProspero::Network
{
    namespace
    {
        struct GuestOption
        {
            int level;
            int name;
        };

        GuestOption ToGuestOption(int level, int option)
        {
            if (level == SOL_SOCKET)
            {
                switch (option)
                {
                case SO_REUSEADDR: return {0xffff, 0x4};
                case SO_REUSEPORT: return {0xffff, 0x200};
                case SO_KEEPALIVE: return {0xffff, 0x8};
                case SO_BROADCAST: return {0xffff, 0x20};
                case SO_LINGER: return {0xffff, 0x80};
                case SO_SNDBUF: return {0xffff, 0x1001};
                case SO_RCVBUF: return {0xffff, 0x1002};
                case SO_ERROR: return {0xffff, 0x1007};
                }
            }
            if (level == IPPROTO_IP)
            {
                switch (option)
                {
                case IP_TOS: return {0, 3};
                case IP_TTL: return {0, 4};
                case IP_MULTICAST_IF: return {0, 9};
                case IP_MULTICAST_TTL: return {0, 10};
                case IP_MULTICAST_LOOP: return {0, 11};
                case IP_ADD_MEMBERSHIP: return {0, 12};
                case IP_DROP_MEMBERSHIP: return {0, 13};
                }
            }
            if (level == IPPROTO_IPV6)
            {
                switch (option)
                {
                case IPV6_MULTICAST_IF: return {41, 9};
                case IPV6_MULTICAST_HOPS: return {41, 10};
                case IPV6_MULTICAST_LOOP: return {41, 11};
                case IPV6_JOIN_GROUP: return {41, 12};
                case IPV6_LEAVE_GROUP: return {41, 13};
                case IPV6_V6ONLY: return {41, 27};
                }
            }
            if (level == IPPROTO_TCP && option == TCP_NODELAY) return {6, 1};
            throw std::invalid_argument("Unsupported PS5 socket option");
        }
    }

    int SetSocketOption(int descriptor, int level, int option, const void* value, socklen_t length)
    {
        const auto guest = ToGuestOption(level, option);
        return setsockopt_nid_postfix(descriptor, guest.level, guest.name, value, length);
    }

    int GetSocketOption(int descriptor, int level, int option, void* value, socklen_t* length)
    {
        const auto guest = ToGuestOption(level, option);
        return getsockopt_nid_postfix(descriptor, guest.level, guest.name, value, length);
    }

    int SocketIoctl(int descriptor, unsigned long request, void* argument)
    {
        switch (request)
        {
        case FIONBIO: return ioctl_nid_postfix(descriptor, 0x8004667e, argument);
        case FIONREAD: return ioctl_nid_postfix(descriptor, 0x4004667f, argument);
        default: throw std::invalid_argument("Unsupported PS5 socket ioctl");
        }
    }
}
