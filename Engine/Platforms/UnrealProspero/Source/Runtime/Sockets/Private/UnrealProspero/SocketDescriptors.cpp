#include "UnrealProspero/SocketDescriptors.h"
#include "UnrealProspero/SocketAddress.h"
#include "prx/libkernel/include/Api.hpp"
#include <limits>
#include <stdexcept>
#include <vector>

namespace UnrealProspero::Network
{
    namespace
    {
        struct PollFlag
        {
            short source;
            short guest;
        };

        constexpr PollFlag PollFlags[] = {
            {POLLIN, 0x01}, {POLLPRI, 0x02}, {POLLOUT, 0x04}, {POLLERR, 0x08}, {POLLHUP, 0x10},
            {POLLNVAL, 0x20}, {POLLRDNORM, 0x40}, {POLLRDBAND, 0x80}, {POLLWRBAND, 0x100}
        };

        short TranslatePollFlags(short flags, bool toGuest)
        {
            short result = 0;
            for (const auto& flag : PollFlags)
            {
                const auto input = toGuest ? flag.source : flag.guest;
                if (flags & input)
                {
                    result |= toGuest ? flag.guest : flag.source;
                    flags &= static_cast<short>(~input);
                }
            }
            if (toGuest && flags == POLLWRNORM) return result | 0x04;
            if (flags) throw std::invalid_argument("Unsupported socket poll flags");
            return result;
        }
    }

    int OpenSocket(int family, int type, int protocol)
    {
        const int baseType = type & ~(SOCK_CLOEXEC | SOCK_NONBLOCK);
        if (baseType != SOCK_STREAM && baseType != SOCK_DGRAM) throw std::invalid_argument("Unsupported PS5 socket type");
        const int guestType = (baseType == SOCK_STREAM ? 1 : 2) | ((type & SOCK_CLOEXEC) ? 0x10000000 : 0) | ((type & SOCK_NONBLOCK) ? 0x20000000 : 0);
        return socket_nid_postfix(SocketAddress::ToGuestFamily(family), guestType, protocol);
    }

    int CloseSocket(int descriptor)
    {
        return close_nid_postfix(descriptor);
    }

    int Listen(int descriptor, int backlog)
    {
        return listen_nid_postfix(descriptor, backlog);
    }

    int Shutdown(int descriptor, int mode)
    {
        static_assert(SHUT_RD == 0 && SHUT_WR == 1 && SHUT_RDWR == 2);
        return shutdown_nid_postfix(descriptor, mode);
    }

    int Poll(pollfd* descriptors, nfds_t count, int timeout)
    {
        if (count > std::numeric_limits<std::uint32_t>::max()) throw std::length_error("Too many PS5 poll descriptors");
        if (count && !descriptors) throw std::invalid_argument("Missing poll descriptors");
        std::vector<GuestPollDescriptor> guest;
        guest.reserve(count);
        for (nfds_t index = 0; index < count; ++index)
        {
            const auto& descriptor = descriptors[index];
            guest.push_back({descriptor.fd, TranslatePollFlags(descriptor.events, true), 0});
        }
        const int result = poll_nid_postfix(guest.data(), static_cast<std::uint32_t>(guest.size()), timeout);
        if (result < 0) return result;
        for (nfds_t index = 0; index < count; ++index) descriptors[index].revents = TranslatePollFlags(guest[index].revents, false);
        return result;
    }

    int ParseInternetAddress(int family, const char* text, void* address)
    {
        return inet_pton_nid_postfix(SocketAddress::ToGuestFamily(family), text, address);
    }
}
