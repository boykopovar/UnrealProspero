#include "Response.h"
#include "Containers/StringConv.h"
#include <charconv>
#include <limits>
#include <stdexcept>

namespace UnrealProspero::Http
{
    Response::Response(const FHttpRequestCommon& request) : FHttpResponseCommon(request) {}

    void Response::SetNativeHeaders(int32 status, std::string_view rawHeaders)
    {
        if (rawHeaders.size() > static_cast<std::size_t>(std::numeric_limits<int32>::max())) throw std::length_error("HTTP response headers are too large");

        SetResponseCode(status);
        FUTF8ToTCHAR converted(rawHeaders.data(), static_cast<int32>(rawHeaders.size()));
        FString text(converted.Length(), converted.Get());
        TArray<FString> lines;
        text.ParseIntoArrayLines(lines);
        for (const FString& line : lines)
        {
            if (line.StartsWith(TEXT("HTTP/"))) continue;
            FString name;
            FString value;
            if (!line.Split(TEXT(":"), &name, &value) || name.IsEmpty()) throw std::runtime_error("Invalid HTTP response header");
            value.TrimStartAndEndInline();
            headerLines.Add(name + TEXT(": ") + value);
            FString& stored = Headers.FindOrAdd(name);
            if (!stored.IsEmpty()) stored += TEXT(", ");
            stored += value;
        }
        bIsReady = true;
    }

    void Response::AppendBody(const uint8* data, int32 size)
    {
        AppendToPayload(data, size);
    }

    TArray<FString> Response::GetAllHeaders() const
    {
        return headerLines;
    }

    FString Response::GetContentType() const
    {
        return GetHeader(TEXT("Content-Type"));
    }

    uint64 Response::GetContentLength() const
    {
        const FString length = GetHeader(TEXT("Content-Length"));
        uint64 value = 0;
        if (length.IsEmpty()) return 0;
        FTCHARToUTF8 converted(*length);
        const char* end = converted.Get() + converted.Length();
        const auto result = std::from_chars(converted.Get(), end, value);
        if (result.ec != std::errc{} || result.ptr != end) throw std::runtime_error("Invalid HTTP content length");
        return value;
    }
}
