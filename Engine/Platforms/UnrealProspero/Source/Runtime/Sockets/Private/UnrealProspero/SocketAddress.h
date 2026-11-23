#ifndef UNREAL_PROSPERO_SOCKET_ADDRESS_H
#define UNREAL_PROSPERO_SOCKET_ADDRESS_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <sys/socket.h>

namespace UnrealProspero::Network
{
    class SocketAddress
    {
    public:
        static constexpr int Ipv4 = 2;
        static constexpr int Ipv6 = 28;

        static int ToGuestFamily(int family);
        static int FromGuestFamily(int family);
        static SocketAddress Encode(const sockaddr& address, socklen_t length);
        void Decode(sockaddr& address, socklen_t& length) const;
        const void* Data() const;
        void* Data();
        std::uint32_t Size() const;
        std::uint32_t& Size();

    private:
        alignas(std::uint32_t) std::array<std::byte, 128> bytes{};
        std::uint32_t length = static_cast<std::uint32_t>(bytes.size());
    };
}

#endif
