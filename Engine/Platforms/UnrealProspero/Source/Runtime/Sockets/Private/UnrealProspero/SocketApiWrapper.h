#ifndef UNREAL_PROSPERO_SOCKET_API_WRAPPER_H
#define UNREAL_PROSPERO_SOCKET_API_WRAPPER_H

#include "UnrealProspero/SocketApi.h"
#include "prx/libkernel/include/Api.hpp"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#define USE_SOCKET_FEATURE_POLL 1
#undef O_NONBLOCK
#define O_NONBLOCK 0x4
#undef IP_PKTINFO

#define socket(...) UnrealProspero::Network::OpenSocket(__VA_ARGS__)
#define close(...) UnrealProspero::Network::CloseSocket(__VA_ARGS__)
#define shutdown(...) UnrealProspero::Network::Shutdown(__VA_ARGS__)
#define bind(...) UnrealProspero::Network::Bind(__VA_ARGS__)
#define connect(...) UnrealProspero::Network::Connect(__VA_ARGS__)
#define listen(...) UnrealProspero::Network::Listen(__VA_ARGS__)
#define accept(...) UnrealProspero::Network::Accept(__VA_ARGS__)
#define sendto(...) UnrealProspero::Network::SendTo(__VA_ARGS__)
#define send(...) UnrealProspero::Network::Send(__VA_ARGS__)
#define recvfrom(...) UnrealProspero::Network::ReceiveFrom(__VA_ARGS__)
#define recv(...) UnrealProspero::Network::Receive(__VA_ARGS__)
#define getsockname(...) UnrealProspero::Network::GetSocketName(__VA_ARGS__)
#define getpeername(...) UnrealProspero::Network::GetPeerName(__VA_ARGS__)
#define setsockopt(...) UnrealProspero::Network::SetSocketOption(__VA_ARGS__)
#define getsockopt(...) UnrealProspero::Network::GetSocketOption(__VA_ARGS__)
#define ioctlsocket(...) UnrealProspero::Network::SocketIoctl(__VA_ARGS__)
#define fcntl(...) fcntl_nid_postfix(__VA_ARGS__)
#define poll(...) UnrealProspero::Network::Poll(__VA_ARGS__)
#define getaddrinfo(...) UnrealProspero::Network::ResolveAddress(__VA_ARGS__)
#define freeaddrinfo(...) UnrealProspero::Network::FreeAddresses(__VA_ARGS__)
#define getnameinfo(...) UnrealProspero::Network::ResolveName(__VA_ARGS__)
#define inet_pton(...) UnrealProspero::Network::ParseInternetAddress(__VA_ARGS__)
#define gethostname(...) UnrealProspero::Network::ReadHostName(__VA_ARGS__)

#endif
