#include "Request.h"
#include "RequestHandle.h"
#include "Response.h"
#include "Async/Async.h"
#include "Http.h"
#include "HttpManager.h"
#include "HttpModule.h"
#include <array>
#include <cmath>
#include <limits>
#include "Containers/StringConv.h"
#include <stdexcept>
#include <utility>

namespace UnrealProspero::Http
{
    Request::Request(std::shared_ptr<const Context> context) : context(std::move(context))
    {
        if (!this->context) throw std::invalid_argument("HTTP context is required");
        bUsePlatformActivityTimeout = false;
    }

    FString Request::GetVerb() const
    {
        return verb;
    }

    void Request::SetVerb(const FString& requestVerb)
    {
        if (GetStatus() == EHttpRequestStatus::Processing || requestVerb.IsEmpty()) throw std::invalid_argument("Cannot set HTTP request verb");
        verb = requestVerb;
    }

    void Request::SetContent(const TArray<uint8>& content)
    {
        SetContent(TArray<uint8>(content));
    }

    void Request::SetContent(TArray<uint8>&& content)
    {
        if (GetStatus() == EHttpRequestStatus::Processing) throw std::logic_error("HTTP request is in progress");
        RequestPayload = MakeUnique<FRequestPayloadInMemory>(MoveTemp(content));
    }

    void Request::SetContentAsString(const FString& content)
    {
        FTCHARToUTF8 utf8(*content, content.Len());
        SetContent(TArray<uint8>(reinterpret_cast<const uint8*>(utf8.Get()), utf8.Length()));
    }

    bool Request::SetContentAsStreamedFile(const FString& filename)
    {
        return SetContentAsStreamedFileDefaultImpl(filename);
    }

    bool Request::SetContentFromStream(TSharedRef<FArchive, ESPMode::ThreadSafe> stream)
    {
        if (GetStatus() == EHttpRequestStatus::Processing) throw std::logic_error("HTTP request is in progress");
        RequestPayload = MakeUnique<FRequestPayloadInFileStream>(MoveTemp(stream));
        return true;
    }

    void Request::AppendToHeader(const FString& name, const FString& value)
    {
        const FString previous = GetHeader(name);
        SetHeader(name, previous.IsEmpty() ? value : previous + TEXT(", ") + value);
    }

    TArray<FString> Request::GetAllHeaders() const
    {
        TArray<FString> headers;
        headers.Reserve(RequestHeaders.Num());
        for (const auto& entry : RequestHeaders) headers.Add(entry.Key + TEXT(": ") + entry.Value);
        return headers;
    }

    FString Request::GetContentType() const
    {
        return GetHeader(TEXT("Content-Type"));
    }

    uint64 Request::GetContentLength() const
    {
        return RequestPayload ? RequestPayload->GetContentLength() : 0;
    }

    const TArray<uint8>& Request::GetContent() const
    {
        static const TArray<uint8> empty;
        return RequestPayload ? RequestPayload->GetContent() : empty;
    }

    bool Request::ProcessRequest()
    {
        if (GetStatus() == EHttpRequestStatus::Processing) return false;
        ResponseCommon.Reset();
        if (!PreProcess()) return false;
        SetStatus(EHttpRequestStatus::Processing);
        FHttpModule::Get().GetHttpManager().AddThreadedRequest(SharedThis(this));
        return true;
    }

    bool Request::SetupRequest()
    {
        return OpenRequestPayloadDefaultImpl();
    }

    bool Request::StartThreadedRequest()
    {
        if (bCanceled || !FHttpRequestCommon::StartThreadedRequest()) return false;
        StartActivityTimeoutTimer();
        task = Async(EAsyncExecution::ThreadPool, [self = SharedThis(this)] { return self->Execute(); });
        return true;
    }

    TSharedPtr<Response> Request::Execute()
    {
        try
        {
            const auto size = GetContentLength();
            if (size > std::numeric_limits<int32>::max()) throw std::length_error("HTTP request body exceeds the in-memory limit");
            TArray<uint8> body;
            body.SetNumUninitialized(static_cast<int32>(size));
            std::size_t offset = 0;
            while (offset < size)
            {
                if (bCanceled || bTimedOut || bActivityTimedOut) return {};
                const auto count = RequestPayload->FillOutputBuffer(body.GetData() + offset, size - offset, offset);
                if (!count || count > size - offset) throw std::runtime_error("Invalid HTTP request body length");
                offset += count;
            }

            FTCHARToUTF8 userAgent(*GetHeader(TEXT("User-Agent")));
            FTCHARToUTF8 method(*verb);
            FTCHARToUTF8 address(*URL);
            auto native = std::make_shared<RequestHandle>(context, userAgent.Get(), method.Get(), address.Get(), size);
            for (const auto& entry : RequestHeaders)
            {
                if (entry.Key.Equals(TEXT("User-Agent"), ESearchCase::IgnoreCase)) continue;
                FTCHARToUTF8 name(*entry.Key);
                FTCHARToUTF8 value(*entry.Value);
                native->SetHeader(name.Get(), value.Get());
            }
            const double timeout = std::ceil(static_cast<double>(GetTimeoutOrDefault()) * 1000000.0);
            if (timeout > 0)
            {
                if (timeout > std::numeric_limits<std::uint32_t>::max()) throw std::out_of_range("HTTP timeout exceeds the native limit");
                native->SetTimeout(static_cast<std::uint32_t>(timeout));
            }
            {
                const std::lock_guard lock(handleMutex);
                if (bCanceled || bTimedOut || bActivityTimedOut) return {};
                handle = native;
            }

            native->Send(std::as_bytes(std::span(body.GetData(), static_cast<std::size_t>(size))));
            ResetActivityTimeoutTimer(TEXTVIEW("Response headers"));
            auto response = MakeShared<Response>(*this);
            response->SetNativeHeaders(native->GetStatusCode(), native->GetHeaders());
            std::array<std::byte, 16384> buffer;
            while (!bCanceled && !bTimedOut && !bActivityTimedOut)
            {
                const auto count = native->Read(buffer);
                if (!count) return response;
                if (count > buffer.size()) throw std::runtime_error("Invalid native HTTP read count");
                ResetActivityTimeoutTimer(TEXTVIEW("Response data"));
                if (bInitializedWithValidStream)
                {
                    if (!PassReceivedDataToStream(buffer.data(), count)) return {};
                }
                else response->AppendBody(reinterpret_cast<const uint8*>(buffer.data()), static_cast<int32>(count));
            }
        }
        catch (const std::exception& error)
        {
            UE_LOG(LogHttp, Warning, TEXT("Native HTTP request failed: %s"), UTF8_TO_TCHAR(error.what()));
        }
        return {};
    }

    void Request::TickThreadedRequest(float)
    {
    }

    bool Request::IsThreadedRequestComplete()
    {
        return !task.IsValid() || task.IsReady();
    }

    void Request::FinishRequest()
    {
        if (task.IsValid()) ResponseCommon = task.Get();
        StopActivityTimeoutTimer();
        PostProcess();
        const bool succeeded = ResponseCommon.IsValid() && !bCanceled && !bTimedOut && !bActivityTimedOut;
        if (succeeded)
        {
            StatusCodeReceivedDelegate.ExecuteIfBound(SharedThis(this), ResponseCommon->GetResponseCode());
            BroadcastResponseHeadersReceived();
        }
        OnFinishRequest(succeeded);
    }

    void Request::Tick(float)
    {
    }

    void Request::AbortRequest()
    {
        const std::lock_guard lock(handleMutex);
        if (handle)
        {
            try
            {
                handle->Abort();
            }
            catch (const std::exception& error)
            {
                UE_LOG(LogHttp, Warning, TEXT("Native HTTP abort failed: %s"), UTF8_TO_TCHAR(error.what()));
            }
        }
    }

    void Request::CleanupRequest()
    {
        CloseRequestPayloadDefaultImpl();
        const std::lock_guard lock(handleMutex);
        handle.reset();
    }

    FHttpResponsePtr Request::CreateResponse()
    {
        return MakeShared<Response>(*this);
    }

    void Request::MockResponseData()
    {
    }
}
