#include "UnrealProspero/UnrealProsperoPlatformHttp.h"
#include "Context.h"
#include "Request.h"
#include <mutex>
#include <stdexcept>

namespace
{
    std::mutex httpContextMutex;
    std::shared_ptr<const UnrealProspero::Http::Context> httpContext;
}

void FUnrealProsperoPlatformHttp::Init()
{
    const std::lock_guard lock(httpContextMutex);
    if (httpContext) throw std::logic_error("HTTP platform is already initialized");
    httpContext = std::make_shared<UnrealProspero::Http::Context>(32);
}

void FUnrealProsperoPlatformHttp::Shutdown()
{
    const std::lock_guard lock(httpContextMutex);
    httpContext.reset();
}

IHttpRequest* FUnrealProsperoPlatformHttp::ConstructRequest()
{
    const std::lock_guard lock(httpContextMutex);
    return new UnrealProspero::Http::Request(httpContext);
}
