#ifndef UNREAL_PROSPERO_SOCKET_ENDPOINTS_H
#define UNREAL_PROSPERO_SOCKET_ENDPOINTS_H

#include <cstddef>
#include <sys/socket.h>

namespace UnrealProspero::Network
{
    int Bind(int descriptor, const sockaddr* address, socklen_t length);
    int Connect(int descriptor, const sockaddr* address, socklen_t length);
    int Accept(int descriptor, sockaddr* address, socklen_t* length);
    int GetSocketName(int descriptor, sockaddr* address, socklen_t* length);
    int GetPeerName(int descriptor, sockaddr* address, socklen_t* length);
    ssize_t Send(int descriptor, const void* data, std::size_t size, int flags);
    ssize_t SendTo(int descriptor, const void* data, std::size_t size, int flags, const sockaddr* address, socklen_t length);
    ssize_t Receive(int descriptor, void* data, std::size_t size, int flags);
    ssize_t ReceiveFrom(int descriptor, void* data, std::size_t size, int flags, sockaddr* address, socklen_t* length);
}

#endif
