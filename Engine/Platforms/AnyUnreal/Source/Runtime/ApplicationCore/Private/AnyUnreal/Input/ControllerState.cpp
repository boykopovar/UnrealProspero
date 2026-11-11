#include "AnyUnreal/Input/ControllerState.h"
#include <cmath>
#include <stdexcept>

namespace AnyUnreal::Input
{
    ControllerFrame ControllerState::Update(const PadData* sample, bool enabled, double deltaSeconds)
    {
        if (!std::isfinite(deltaSeconds) || deltaSeconds < 0) throw std::invalid_argument("Invalid controller polling interval");
        ControllerFrame frame;
        std::array<bool, 24> next{};
        if (sample && sample->connected && enabled)
        {
            constexpr std::array<unsigned, 16> masks{2, 4, 8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000, 0x100000};
            for (std::size_t index = 0; index < masks.size(); ++index) next[index] = (sample->buttons & masks[index]) != 0;
            const auto normalize = [](unsigned value) { return value < 128 ? (static_cast<float>(value) - 128.0f) / 128.0f : (static_cast<float>(value) - 128.0f) / 127.0f; };
            frame.Axes = {normalize(sample->left_stick_x), -normalize(sample->left_stick_y), normalize(sample->right_stick_x), -normalize(sample->right_stick_y), sample->analog_buttons_l2 / 255.0f, sample->analog_buttons_r2 / 255.0f};
            for (std::size_t stick = 0; stick < 2; ++stick)
            {
                const auto offset = 16 + stick * 4;
                next[offset] = frame.Axes[stick * 2 + 1] > 0.5f;
                next[offset + 1] = frame.Axes[stick * 2 + 1] < -0.5f;
                next[offset + 2] = frame.Axes[stick * 2] > 0.5f;
                next[offset + 3] = frame.Axes[stick * 2] < -0.5f;
            }
        }
        for (std::size_t index = 0; index < next.size(); ++index)
        {
            if (next[index] != buttons[index])
            {
                frame.Buttons[index] = next[index] ? ButtonChange::Press : ButtonChange::Release;
                repeatRemaining[index] = next[index] ? 0.2 : 0;
            }
            else if (next[index])
            {
                repeatRemaining[index] -= deltaSeconds;
                if (repeatRemaining[index] <= 0)
                {
                    frame.Buttons[index] = ButtonChange::Repeat;
                    repeatRemaining[index] = 0.1;
                }
            }
        }
        for (std::size_t index = 0; index < axes.size(); ++index) frame.SendAxes[index] = frame.Axes[index] != 0 || frame.Axes[index] != axes[index];
        buttons = next;
        axes = frame.Axes;
        return frame;
    }
}
