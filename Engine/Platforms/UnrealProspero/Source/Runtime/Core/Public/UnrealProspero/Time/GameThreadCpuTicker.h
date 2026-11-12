#ifndef UNREALPROSPERO_TIME_GAME_THREAD_CPU_TICKER_H
#define UNREALPROSPERO_TIME_GAME_THREAD_CPU_TICKER_H

namespace UnrealProspero::Time
{
    class GameThreadCpuTicker final
    {
    public:
        static void Enable(double updateInterval);
    };
}

#endif
