#include "Context.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libSceHttp2/include/Http2.hpp"
#include "prx/libSceNet/include/Net.hpp"
#include "prx/libSceSsl/include/Ssl.hpp"
#include <algorithm>
#include <cstddef>
#include <exception>
#include <limits>
#include <stdexcept>

namespace UnrealProspero::Http
{
    Context::Context(int maxRequests) : Context()
    {
        if (maxRequests <= 0) throw std::invalid_argument("HTTP request limit must be positive");
        constexpr std::size_t memoryBlockSize = 256 * 1024;
        constexpr std::size_t requestsPerBlock = 3;
        const auto blocks = (static_cast<std::size_t>(maxRequests) + requestsPerBlock - 1) / requestsPerBlock;
        if (blocks > std::numeric_limits<std::size_t>::max() / memoryBlockSize) throw std::overflow_error("HTTP request pool size exceeds address space");
        const auto poolSize = blocks * memoryBlockSize;

        Kernel::CheckResult(sceNetInit_nid_postfix(), "sceNetInit");
        networkInitialized = true;

        networkPool = sceNetPoolCreate("UnrealProsperoHttp", 1024 * 1024, 0);
        if (networkPool < 0) throw Kernel::Error(networkPool, "sceNetPoolCreate");

        ssl = sceSslInit_nid_postfix(std::max(std::size_t{8 * 1024 * 1024}, poolSize));
        if (ssl < 0) throw Kernel::Error(ssl, "sceSslInit");

        http = sceHttp2Init(networkPool, ssl, poolSize, maxRequests);
        if (http < 0) throw Kernel::Error(http, "sceHttp2Init");
    }

    Context::~Context() noexcept
    {
        const int httpResult = http >= 0 ? sceHttp2Term(http) : 0;
        const int sslResult = ssl >= 0 ? sceSslTerm_nid_postfix(ssl) : 0;
        const int poolResult = networkPool >= 0 ? sceNetPoolDestroy(networkPool) : 0;
        const int netResult = networkInitialized ? sceNetTerm() : 0;
        if (httpResult != 0 || sslResult != 0 || poolResult != 0 || netResult != 0) std::terminate();
    }

    int Context::GetId() const
    {
        return http;
    }
}
