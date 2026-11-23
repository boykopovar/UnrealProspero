#ifndef UNREALPROSPERO_PLATFORM_HTTP_H
#define UNREALPROSPERO_PLATFORM_HTTP_H

#include "GenericPlatform/GenericPlatformHttp.h"

class FUnrealProsperoPlatformHttp : public FGenericPlatformHttp
{
public:
    static HTTP_API void Init();
    static HTTP_API void Shutdown();
    static HTTP_API IHttpRequest* ConstructRequest();
};

using FPlatformHttp = FUnrealProsperoPlatformHttp;

#endif
