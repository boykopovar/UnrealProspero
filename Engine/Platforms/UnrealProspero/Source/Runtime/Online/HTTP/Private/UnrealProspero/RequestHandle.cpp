#include "RequestHandle.h"
#include "Context.h"
#include "UnrealProspero/Kernel/Error.h"
#include "prx/libSceHttp2/include/Http2.hpp"
#include <exception>
#include <stdexcept>
#include <utility>

namespace UnrealProspero::Http
{
    RequestHandle::RequestHandle(std::shared_ptr<const Context> context) : context(std::move(context)) {}

    RequestHandle::RequestHandle(std::shared_ptr<const Context> context, const char* userAgent, const char* method, const char* url, std::uint64_t contentLength) : RequestHandle(std::move(context))
    {
        if (!this->context) throw std::invalid_argument("HTTP context is required");

        constexpr int http2Version = 3;
        requestTemplate = sceHttp2CreateTemplate(this->context->GetId(), userAgent, http2Version, 0);
        if (requestTemplate < 0) throw Kernel::Error(requestTemplate, "sceHttp2CreateTemplate");

        request = sceHttp2CreateRequestWithURL(requestTemplate, method, url, contentLength);
        if (request < 0) throw Kernel::Error(request, "sceHttp2CreateRequestWithURL");
    }

    RequestHandle::~RequestHandle() noexcept
    {
        const int requestResult = request >= 0 ? sceHttp2DeleteRequest(request) : 0;
        const int templateResult = requestTemplate >= 0 ? sceHttp2DeleteTemplate(requestTemplate) : 0;
        if (requestResult != 0 || templateResult != 0) std::terminate();
    }

    int RequestHandle::GetId() const
    {
        return request;
    }
}
