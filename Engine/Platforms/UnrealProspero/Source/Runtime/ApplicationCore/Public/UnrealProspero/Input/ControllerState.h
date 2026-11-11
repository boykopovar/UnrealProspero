#ifndef UNREALPROSPERO_INPUT_CONTROLLER_STATE_H
#define UNREALPROSPERO_INPUT_CONTROLLER_STATE_H

#include "SceTypes.hpp"
#include <array>
#include <cstddef>

namespace UnrealProspero::Input
{
    enum class ButtonChange
    {
        None,
        Press,
        Release,
        Repeat
    };

    struct ControllerFrame
    {
        std::array<ButtonChange, 24> Buttons{};
        std::array<float, 6> Axes{};
        std::array<bool, 6> SendAxes{};
    };

    class APPLICATIONCORE_API ControllerState final
    {
    public:
        ControllerFrame Update(const PadData* sample, bool enabled, double deltaSeconds);

    private:
        std::array<bool, 24> buttons{};
        std::array<double, 24> repeatRemaining{};
        std::array<float, 6> axes{};
    };
}

#endif
