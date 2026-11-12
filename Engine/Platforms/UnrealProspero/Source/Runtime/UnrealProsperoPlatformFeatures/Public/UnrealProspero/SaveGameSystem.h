#ifndef UNREALPROSPERO_SAVE_GAME_SYSTEM_H
#define UNREALPROSPERO_SAVE_GAME_SYSTEM_H

#include <SaveGameSystem.h>
#include "UnrealProspero/Mutex/UnrealProsperoRecursiveMutex.h"

namespace UnrealProspero
{
    class SaveGameSystem final : public ISaveGameSystem
    {
    public:
        SaveGameSystem();
        bool PlatformHasNativeUI() override;
        bool DoesSaveSystemSupportMultipleUsers() override;
        bool DoesSaveGameExist(const TCHAR* name, int32 userIndex) override;
        ESaveExistsResult DoesSaveGameExistWithResult(const TCHAR* name, int32 userIndex) override;
        bool GetSaveGameNames(TArray<FString>& names, int32 userIndex) override;
        bool SaveGame(bool attemptUi, const TCHAR* name, int32 userIndex, const TArray<uint8>& data) override;
        bool LoadGame(bool attemptUi, const TCHAR* name, int32 userIndex, TArray<uint8>& data) override;
        bool DeleteGame(bool attemptUi, const TCHAR* name, int32 userIndex) override;

    private:
        UE::FUnrealProsperoRecursiveMutex mutex;
        std::uint64_t blocks;
    };
}

#endif
