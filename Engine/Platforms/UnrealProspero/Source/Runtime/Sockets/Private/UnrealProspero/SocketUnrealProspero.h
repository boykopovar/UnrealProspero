#ifndef UNREAL_PROSPERO_SOCKET_UNREAL_PROSPERO_H
#define UNREAL_PROSPERO_SOCKET_UNREAL_PROSPERO_H

#include "BSDSockets/SocketsBSD.h"

class FSocketUnrealProspero final : public FSocketBSD
{
public:
    using FSocketBSD::FSocketBSD;
    bool SetMulticastLoopback(bool loopback) override;
};

#endif
