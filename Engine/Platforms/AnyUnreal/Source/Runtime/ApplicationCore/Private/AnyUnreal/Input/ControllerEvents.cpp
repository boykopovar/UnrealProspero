#include "AnyUnreal/Input/ControllerEvents.h"
#include <algorithm>
#include <stdexcept>

namespace AnyUnreal::Input
{
    ControllerEvents::ControllerEvents(int initialUser) : initialUser(initialUser)
    {
        if (initialUser < 0) throw std::invalid_argument("Invalid initial controller user");
    }

    void ControllerEvents::Dispatch(const std::vector<ControllerSample>& samples, FGenericApplicationMessageHandler& handler, bool enabled)
    {
        auto& mapper = IPlatformInputDeviceMapper::Get();
        for (const auto& sample : samples)
        {
            if (!devices.contains(sample.UserId))
            {
                Device device;
                device.User = sample.UserId == initialUser ? mapper.GetPlatformUserForUserIndex(0) : mapper.AllocateNewUserId();
                device.Id = mapper.AllocateNewInputDeviceId();
                if (!mapper.Internal_MapInputDeviceToUser(device.Id, device.User, EInputDeviceConnectionState::Disconnected)) throw std::runtime_error("Controller user mapping failed");
                devices.emplace(sample.UserId, device);
            }
        }
        for (auto& [user, device] : devices)
        {
            const auto sample = std::find_if(samples.begin(), samples.end(), [user](const auto& value) { return value.UserId == user; });
            dispatchDevice(device, sample == samples.end() ? nullptr : &sample->Data, handler, enabled);
        }
    }

    void ControllerEvents::dispatchDevice(Device& device, const PadData* sample, FGenericApplicationMessageHandler& handler, bool enabled)
    {
        const bool connected = sample && sample->connected;
        if (connected && !device.Connected)
        {
            if (!IPlatformInputDeviceMapper::Get().Internal_SetInputDeviceConnectionState(device.Id, EInputDeviceConnectionState::Connected)) throw std::runtime_error("Controller connection update failed");
        }
        const std::array<const FName*, 16> names{&FGamepadKeyNames::LeftThumb, &FGamepadKeyNames::RightThumb, &FGamepadKeyNames::SpecialRight, &FGamepadKeyNames::DPadUp, &FGamepadKeyNames::DPadRight, &FGamepadKeyNames::DPadDown, &FGamepadKeyNames::DPadLeft, &FGamepadKeyNames::LeftTriggerThreshold, &FGamepadKeyNames::RightTriggerThreshold, &FGamepadKeyNames::LeftShoulder, &FGamepadKeyNames::RightShoulder, &FGamepadKeyNames::FaceButtonTop, &FGamepadKeyNames::FaceButtonRight, &FGamepadKeyNames::FaceButtonBottom, &FGamepadKeyNames::FaceButtonLeft, &FGamepadKeyNames::SpecialLeft};
        constexpr std::array<unsigned, 16> masks{2, 4, 8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000, 0x100000};
        for (std::size_t index = 0; index < names.size(); ++index)
        {
            const bool pressed = connected && enabled && (sample->buttons & masks[index]);
            if (pressed != device.Buttons[index])
            {
                if (pressed) handler.OnControllerButtonPressed(*names[index], device.User, device.Id, false);
                else handler.OnControllerButtonReleased(*names[index], device.User, device.Id, false);
                device.Buttons[index] = pressed;
            }
        }
        std::array<float, 6> axes{};
        if (connected && enabled)
        {
            const auto normalize = [](unsigned value) { return value < 128 ? (static_cast<float>(value) - 128.0f) / 128.0f : (static_cast<float>(value) - 128.0f) / 127.0f; };
            axes = {normalize(sample->left_stick_x), -normalize(sample->left_stick_y), normalize(sample->right_stick_x), -normalize(sample->right_stick_y), sample->analog_buttons_l2 / 255.0f, sample->analog_buttons_r2 / 255.0f};
        }
        const std::array<const FName*, 6> axisNames{&FGamepadKeyNames::LeftAnalogX, &FGamepadKeyNames::LeftAnalogY, &FGamepadKeyNames::RightAnalogX, &FGamepadKeyNames::RightAnalogY, &FGamepadKeyNames::LeftTriggerAnalog, &FGamepadKeyNames::RightTriggerAnalog};
        for (std::size_t index = 0; index < axes.size(); ++index)
        {
            if (axes[index] != device.Axes[index]) handler.OnControllerAnalog(*axisNames[index], device.User, device.Id, axes[index]);
        }
        device.Axes = axes;
        if (!connected && device.Connected)
        {
            if (!IPlatformInputDeviceMapper::Get().Internal_SetInputDeviceConnectionState(device.Id, EInputDeviceConnectionState::Disconnected)) throw std::runtime_error("Controller disconnection update failed");
        }
        device.Connected = connected;
    }

    bool ControllerEvents::IsAttached() const
    {
        return std::any_of(devices.begin(), devices.end(), [](const auto& entry) { return entry.second.Connected; });
    }
}
