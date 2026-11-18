#ifndef UNREAL_PROSPERO_SOCKET_OPTIONS_H
#define UNREAL_PROSPERO_SOCKET_OPTIONS_H

#include <sys/socket.h>

namespace UnrealProspero::Network
{
    int SetSocketOption(int descriptor, int level, int option, const void* value, socklen_t length);
    int GetSocketOption(int descriptor, int level, int option, void* value, socklen_t* length);
    int SocketIoctl(int descriptor, unsigned long request, void* argument);
}

#endif
