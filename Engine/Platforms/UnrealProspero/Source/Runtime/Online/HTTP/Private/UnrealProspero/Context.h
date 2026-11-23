#ifndef UNREALPROSPERO_HTTP_CONTEXT_H
#define UNREALPROSPERO_HTTP_CONTEXT_H

namespace UnrealProspero::Http
{
    class Context
    {
    public:
        explicit Context(int maxRequests);
        ~Context() noexcept;
        Context(const Context&) = delete;
        Context& operator=(const Context&) = delete;
        int GetId() const;

    private:
        Context() = default;

        bool networkInitialized = false;
        int networkPool = -1;
        int ssl = -1;
        int http = -1;
    };
}

#endif
