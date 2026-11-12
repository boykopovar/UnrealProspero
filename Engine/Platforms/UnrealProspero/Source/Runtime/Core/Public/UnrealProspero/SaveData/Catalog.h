#ifndef UNREALPROSPERO_SAVE_DATA_CATALOG_H
#define UNREALPROSPERO_SAVE_DATA_CATALOG_H

#include "UnrealProspero/SaveData/TransactionResource.h"
#include <string>
#include <string_view>
#include <vector>

namespace UnrealProspero::SaveData
{
    class Catalog final
    {
    public:
        explicit Catalog(std::uint32_t resourceSize);
        std::vector<std::string> List(int userId) const;
        bool Exists(int userId, std::string_view directory) const;
        void Delete(int userId, std::string_view directory) const;

    private:
        TransactionResource resource;
    };
}

#endif
