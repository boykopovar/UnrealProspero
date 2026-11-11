#ifndef ANYUNREAL_INPUT_CONTROLLER_HUB_H
#define ANYUNREAL_INPUT_CONTROLLER_HUB_H

#include "SceTypes.hpp"
#include <map>
#include <optional>
#include <vector>

namespace AnyUnreal::Input
{
    struct ControllerSample
    {
        int UserId;
        PadData Data;
    };

    class APPLICATIONCORE_API ControllerHub final
    {
    public:
        ControllerHub();
        ~ControllerHub() noexcept(false);
        ControllerHub(const ControllerHub&) = delete;
        ControllerHub& operator=(const ControllerHub&) = delete;
        std::vector<ControllerSample> Poll();
        void Close();
        int GetInitialUser() const;

    private:
        std::map<int, std::optional<int>> handles;
        bool initialized = false;
        int initialUser = -1;
    };
}

#endif
