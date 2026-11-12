#include "UnrealProspero/Time/GameThreadCpuTicker.h"
#include "UnrealProspero/UnrealProsperoPlatformTime.h"
#include "Containers/Ticker.h"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace UnrealProspero::Time
{
    void GameThreadCpuTicker::Enable(double updateInterval)
    {
        if (!std::isfinite(updateInterval) || updateInterval < 0 || updateInterval > std::numeric_limits<float>::max()) throw std::invalid_argument("Invalid CPU sampling interval");
        const float interval = static_cast<float>(updateInterval);
        if (updateInterval > 0 && interval == 0) throw std::invalid_argument("CPU sampling interval is below ticker precision");
        static const auto registration = [interval]
        {
            auto handle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&FUnrealProsperoPlatformTime::UpdateThreadCPUTime), interval);
            if (!handle.IsValid()) throw std::runtime_error("Cannot register game thread CPU sampling");
            return handle;
        }();
        static_cast<void>(registration);
    }
}
