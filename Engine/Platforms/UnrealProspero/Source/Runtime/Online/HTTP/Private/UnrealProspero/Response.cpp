#include "Response.h"
#include "Containers/StringConv.h"
#include <charconv>
#include <limits>
#include <optional>
#include <stdexcept>

namespace UnrealProspero::Http
{
    Response::Response(const FHttpRequestCommon& request) : FHttpResponseCommon(request) {}

    void Response::SetNativeHeaders(int32 status, std::string_view rawHeaders)
    {
        if (rawHeaders.size() > static_cast<std::size_t>(std::numeric_limits<int32>::max())) throw std::length_error("HTTP response headers are too large");

        SetResponseCode(status);
        std::optional<uint64> parsedLength;
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
            if (name.Equals(TEXT("Content-Length"), ESearchCase::IgnoreCase))
            {
                TArray<FString> lengths;
                value.ParseIntoArray(lengths, TEXT(","), false);
                if (lengths.IsEmpty()) throw std::runtime_error("Empty HTTP content length");
                for (FString& length : lengths)
                {
                    length.TrimStartAndEndInline();
                    FTCHARToUTF8 utf8(*length);
                    const char* end = utf8.Get() + utf8.Length();
                    uint64 size = 0;
                    const auto result = std::from_chars(utf8.Get(), end, size);
                    if (result.ec != std::errc{} || result.ptr != end) throw std::runtime_error("Invalid HTTP content length");
                    if (parsedLength && *parsedLength != size) throw std::runtime_error("Conflicting HTTP content lengths");
                    parsedLength = size;
                }
            }

            headerLines.Add(name + TEXT(": ") + value);
            FString& stored = Headers.FindOrAdd(name);
            if (!stored.IsEmpty()) stored += TEXT(", ");
            stored += value;
        }
        contentLength = parsedLength.value_or(0);
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
        return contentLength;
    }
}
