#ifndef UNREALPROSPERO_FILE_TEMPORARY_STORAGE_H
#define UNREALPROSPERO_FILE_TEMPORARY_STORAGE_H

#include "prx/libSceAppContent/include/Api.hpp"
#include <string_view>

namespace UnrealProspero::File
{
    class TemporaryStorage final
    {
    public:
        TemporaryStorage();
        ~TemporaryStorage() noexcept(false);
        TemporaryStorage(const TemporaryStorage&) = delete;
        TemporaryStorage& operator=(const TemporaryStorage&) = delete;
        std::string_view GetPath() const;
        void Close();

    private:
        AppContentMountPoint mountPoint{};
        bool mounted = false;
        bool loaded = false;
    };
}

#endif
