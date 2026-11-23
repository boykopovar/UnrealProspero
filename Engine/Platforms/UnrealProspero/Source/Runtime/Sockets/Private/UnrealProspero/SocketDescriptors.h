#ifndef UNREAL_PROSPERO_SOCKET_DESCRIPTORS_H
#define UNREAL_PROSPERO_SOCKET_DESCRIPTORS_H

#include <poll.h>

namespace UnrealProspero::Network
{
    int OpenSocket(int family, int type, int protocol);
    int CloseSocket(int descriptor);
    int Listen(int descriptor, int backlog);
    int Shutdown(int descriptor, int mode);
    int Poll(pollfd* descriptors, nfds_t count, int timeout);
    int ParseInternetAddress(int family, const char* text, void* address);
}

#endif
