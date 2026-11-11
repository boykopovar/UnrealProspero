#include "UnrealProspero/Input/ControllerState.h"
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

int main()
{
    using namespace UnrealProspero::Input;
    PadData pad{};
    pad.connected = true;
    pad.left_stick_x = pad.left_stick_y = pad.right_stick_x = pad.right_stick_y = 128;
    constexpr std::array<unsigned, 16> masks{2, 4, 8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000, 0x100000};
    for (std::size_t index = 0; index < masks.size(); ++index)
    {
        ControllerState state;
        pad.buttons = masks[index];
        auto frame = state.Update(&pad, true, 0);
        if (frame.Buttons[index] != ButtonChange::Press || std::count(frame.Buttons.begin(), frame.Buttons.end(), ButtonChange::Press) != 1) throw std::runtime_error("Incorrect controller button mapping");
        if (state.Update(&pad, true, 0.1).Buttons[index] != ButtonChange::None) throw std::runtime_error("Controller repeat started early");
        if (state.Update(&pad, true, 0.11).Buttons[index] != ButtonChange::Repeat) throw std::runtime_error("Controller repeat did not start");
        if (state.Update(&pad, true, 0.11).Buttons[index] != ButtonChange::Repeat) throw std::runtime_error("Controller repeat did not continue");
        if (state.Update(&pad, false, 0).Buttons[index] != ButtonChange::Release) throw std::runtime_error("Disabled input did not release a button");
        if (state.Update(&pad, true, 0).Buttons[index] != ButtonChange::Press) throw std::runtime_error("Input activation did not restore a held button");
        if (state.Update(nullptr, true, 0).Buttons[index] != ButtonChange::Release) throw std::runtime_error("Disconnected input did not release a button");
    }
    pad.buttons = 0;
    ControllerState state;
    pad.left_stick_x = 0;
    pad.left_stick_y = 0;
    pad.right_stick_x = 255;
    pad.right_stick_y = 255;
    pad.analog_buttons_l2 = 255;
    pad.analog_buttons_r2 = 0;
    auto frame = state.Update(&pad, true, 0);
    if (frame.Axes != std::array{-1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 0.0f}) throw std::runtime_error("Controller axis endpoints are incorrect");
    for (const auto index : {16, 19, 21, 22}) if (frame.Buttons[index] != ButtonChange::Press) throw std::runtime_error("Stick direction was not pressed");
    frame = state.Update(&pad, true, 0.01);
    for (std::size_t index = 0; index < 5; ++index) if (!frame.SendAxes[index]) throw std::runtime_error("Held analog input was not sent");
    frame = state.Update(nullptr, true, 0);
    if (frame.Axes != std::array<float, 6>{}) throw std::runtime_error("Disconnected axes did not reset");
    for (std::size_t index = 0; index < 5; ++index) if (!frame.SendAxes[index]) throw std::runtime_error("Axis reset was not sent");
    frame = state.Update(nullptr, true, 0);
    if (std::any_of(frame.SendAxes.begin(), frame.SendAxes.end(), [](bool value) { return value; })) throw std::runtime_error("Neutral input generated duplicate axis resets");
    for (const auto interval : {-1.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()})
    {
        bool rejected = false;
        try { state.Update(&pad, true, interval); }
        catch (const std::invalid_argument&) { rejected = true; }
        if (!rejected) throw std::runtime_error("Invalid polling interval was accepted");
    }
    std::cout << "Controller button, axis, repeat and disconnect transitions verified\n";
    return 0;
}
