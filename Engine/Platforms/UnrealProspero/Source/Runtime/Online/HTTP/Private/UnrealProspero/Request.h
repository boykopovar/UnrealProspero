#ifndef UNREALPROSPERO_HTTP_REQUEST_H
#define UNREALPROSPERO_HTTP_REQUEST_H

#include "GenericPlatform/HttpRequestCommon.h"
#include "Async/Future.h"
#include <atomic>
#include <memory>
#include <mutex>

namespace UnrealProspero::Http
{
    class Context;
    class RequestHandle;
    class Response;

    class Request final : public FHttpRequestCommon
    {
    public:
        explicit Request(std::shared_ptr<const Context> context);
        FString GetVerb() const override;
        void SetVerb(const FString& requestVerb) override;
        void SetContent(const TArray<uint8>& content) override;
        void SetContent(TArray<uint8>&& content) override;
        void SetContentAsString(const FString& content) override;
        bool SetContentAsStreamedFile(const FString& filename) override;
        bool SetContentFromStream(TSharedRef<FArchive, ESPMode::ThreadSafe> stream) override;
        void AppendToHeader(const FString& name, const FString& value) override;
        TArray<FString> GetAllHeaders() const override;
        FString GetContentType() const override;
        uint64 GetContentLength() const override;
        const TArray<uint8>& GetContent() const override;
        bool ProcessRequest() override;
        bool StartThreadedRequest() override;
        void TickThreadedRequest(float deltaSeconds) override;
        bool IsThreadedRequestComplete() override;
        void FinishRequest() override;
        void Tick(float deltaSeconds) override;

    private:
        bool SetupRequest() override;
        void AbortRequest() override;
        void CleanupRequest() override;
        FHttpResponsePtr CreateResponse() override;
        void MockResponseData() override;

        TSharedPtr<Response> Execute();
        void reportProgress();

        std::shared_ptr<const Context> context;
        std::mutex handleMutex;
        std::shared_ptr<RequestHandle> handle;
        TFuture<TSharedPtr<Response>> task;
        std::atomic<uint64> bytesSent{0};
        std::atomic<uint64> bytesReceived{0};
        uint64 reportedBytesSent = 0;
        uint64 reportedBytesReceived = 0;
        FString verb = TEXT("GET");
    };
}

#endif
