#ifndef UNREALPROSPERO_HTTP_RESPONSE_H
#define UNREALPROSPERO_HTTP_RESPONSE_H

#include "GenericPlatform/HttpResponseCommon.h"
#include <string_view>

namespace UnrealProspero::Http
{
    class Response final : public FHttpResponseCommon
    {
    public:
        explicit Response(const FHttpRequestCommon& request);
        void SetNativeHeaders(int32 status, std::string_view rawHeaders);
        void AppendBody(const uint8* data, int32 size);
        TArray<FString> GetAllHeaders() const override;
        FString GetContentType() const override;
        uint64 GetContentLength() const override;

    private:
        TArray<FString> headerLines;
        uint64 contentLength = 0;
    };
}

#endif
