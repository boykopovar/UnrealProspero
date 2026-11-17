#ifndef UNREALPROSPERO_HTTP_REQUEST_HANDLE_H
#define UNREALPROSPERO_HTTP_REQUEST_HANDLE_H

#include <cstdint>
#include <memory>

namespace UnrealProspero::Http
{
    class Context;

    class RequestHandle
    {
    public:
        RequestHandle(std::shared_ptr<const Context> context, const char* userAgent, const char* method, const char* url, std::uint64_t contentLength);
        ~RequestHandle() noexcept;
        RequestHandle(const RequestHandle&) = delete;
        RequestHandle& operator=(const RequestHandle&) = delete;
        int GetId() const;

    private:
        explicit RequestHandle(std::shared_ptr<const Context> context);

        std::shared_ptr<const Context> context;
        int requestTemplate = -1;
        int request = -1;
    };
}

#endif
