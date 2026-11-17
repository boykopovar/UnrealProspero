#include "UnrealProspero/UnrealProsperoPlatformHttp.h"
#include "Context.h"
#include "Request.h"
#include <stdexcept>

namespace
{
    std::shared_ptr<const UnrealProspero::Http::Context> httpContext;
}

void FUnrealProsperoPlatformHttp::Init()
{
    if (httpContext) throw std::logic_error("HTTP platform is already initialized");
    httpContext = std::make_shared<UnrealProspero::Http::Context>(32);
}

void FUnrealProsperoPlatformHttp::Shutdown()
{
    httpContext.reset();
}

IHttpRequest* FUnrealProsperoPlatformHttp::ConstructRequest()
{
    return new UnrealProspero::Http::Request(httpContext);
}
