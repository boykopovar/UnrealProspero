#include "RequestHandle.h"
#include "Context.h"
#include "UnrealProspero/Kernel/Result.h"
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

    void RequestHandle::SetHeader(const char* name, const char* value) const
    {
        constexpr std::uint32_t replace = 0;
        Kernel::CheckResult(sceHttp2AddRequestHeader(request, name, value, replace), "sceHttp2AddRequestHeader");
    }

    void RequestHandle::SetTimeout(std::uint32_t microseconds) const
    {
        if (!microseconds) throw std::invalid_argument("HTTP timeout must be positive");
        Kernel::CheckResult(sceHttp2SetTimeOut(request, microseconds), "sceHttp2SetTimeOut");
    }

    void RequestHandle::Send(std::span<const std::byte> body) const
    {
        Kernel::CheckResult(sceHttp2SendRequest(request, body.data(), body.size()), "sceHttp2SendRequest");
    }

    void RequestHandle::Abort() const
    {
        Kernel::CheckResult(sceHttp2AbortRequest(request), "sceHttp2AbortRequest");
    }

    int RequestHandle::GetStatusCode() const
    {
        int status = 0;
        Kernel::CheckResult(sceHttp2GetStatusCode(request, &status), "sceHttp2GetStatusCode");
        return status;
    }

    std::string RequestHandle::GetHeaders() const
    {
        char* data = nullptr;
        std::size_t size = 0;
        Kernel::CheckResult(sceHttp2GetAllResponseHeaders(request, &data, &size), "sceHttp2GetAllResponseHeaders");
        if (size == 0) return {};
        if (!data) throw std::runtime_error("HTTP response headers have no data");
        if (data[size - 1] == '\0') --size;
        return std::string(data, size);
    }

    std::size_t RequestHandle::Read(std::span<std::byte> buffer) const
    {
        if (buffer.empty()) throw std::invalid_argument("HTTP read buffer is empty");
        const int result = sceHttp2ReadData(request, buffer.data(), buffer.size());
        if (result < 0) throw Kernel::Error(result, "sceHttp2ReadData");
        return static_cast<std::size_t>(result);
    }
}
