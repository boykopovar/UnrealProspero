#ifndef UNREALPROSPERO_HTTP_REQUEST_HANDLE_H
#define UNREALPROSPERO_HTTP_REQUEST_HANDLE_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>

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
        void SetHeader(const char* name, const char* value) const;
        void Send(std::span<const std::byte> body) const;
        void Abort() const;
        int GetStatusCode() const;
        std::string GetHeaders() const;
        std::size_t Read(std::span<std::byte> buffer) const;

    private:
        explicit RequestHandle(std::shared_ptr<const Context> context);

        std::shared_ptr<const Context> context;
        int requestTemplate = -1;
        int request = -1;
    };
}

#endif
