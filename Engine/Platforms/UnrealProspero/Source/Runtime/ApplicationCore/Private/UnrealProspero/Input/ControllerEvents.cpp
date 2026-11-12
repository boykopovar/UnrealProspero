#include "UnrealProspero/Input/ControllerEvents.h"
#include "UnrealProspero/Kernel/UserService.h"
#include <algorithm>
#include <stdexcept>

namespace UnrealProspero::Input
{
    ControllerEvents::ControllerEvents(int initialUser)
    {
        Kernel::UserService users;
        if (users.GetUserId(0) != initialUser) throw std::invalid_argument("Invalid initial controller user");
    }

    void ControllerEvents::Dispatch(const std::vector<ControllerSample>& samples, FGenericApplicationMessageHandler& handler, bool enabled, double deltaSeconds)
    {
        auto& mapper = IPlatformInputDeviceMapper::Get();
        Kernel::UserService users;
        for (const auto& sample : samples)
        {
            if (!devices.contains(sample.UserId))
            {
                Device device;
                device.User = FPlatformUserId::CreateFromInternalId(users.GetUserIndex(sample.UserId));
                device.Id = mapper.AllocateNewInputDeviceId();
                if (!mapper.Internal_MapInputDeviceToUser(device.Id, device.User, EInputDeviceConnectionState::Disconnected)) throw std::runtime_error("Controller user mapping failed");
                devices.emplace(sample.UserId, device);
            }
        }
        for (auto& [user, device] : devices)
        {
            const auto sample = std::find_if(samples.begin(), samples.end(), [user](const auto& value) { return value.UserId == user; });
            dispatchDevice(device, sample == samples.end() ? nullptr : &sample->Data, handler, enabled, deltaSeconds);
        }
    }

    void ControllerEvents::dispatchDevice(Device& device, const PadData* sample, FGenericApplicationMessageHandler& handler, bool enabled, double deltaSeconds)
    {
        const bool connected = sample && sample->connected;
        if (connected && !device.Connected)
        {
            if (!IPlatformInputDeviceMapper::Get().Internal_SetInputDeviceConnectionState(device.Id, EInputDeviceConnectionState::Connected)) throw std::runtime_error("Controller connection update failed");
        }
        const auto frame = device.State.Update(sample, enabled, deltaSeconds);
        const std::array<const FName*, 24> names{&FGamepadKeyNames::LeftThumb, &FGamepadKeyNames::RightThumb, &FGamepadKeyNames::SpecialRight, &FGamepadKeyNames::DPadUp, &FGamepadKeyNames::DPadRight, &FGamepadKeyNames::DPadDown, &FGamepadKeyNames::DPadLeft, &FGamepadKeyNames::LeftTriggerThreshold, &FGamepadKeyNames::RightTriggerThreshold, &FGamepadKeyNames::LeftShoulder, &FGamepadKeyNames::RightShoulder, &FGamepadKeyNames::FaceButtonTop, &FGamepadKeyNames::FaceButtonRight, &FGamepadKeyNames::FaceButtonBottom, &FGamepadKeyNames::FaceButtonLeft, &FGamepadKeyNames::SpecialLeft, &FGamepadKeyNames::LeftStickUp, &FGamepadKeyNames::LeftStickDown, &FGamepadKeyNames::LeftStickRight, &FGamepadKeyNames::LeftStickLeft, &FGamepadKeyNames::RightStickUp, &FGamepadKeyNames::RightStickDown, &FGamepadKeyNames::RightStickRight, &FGamepadKeyNames::RightStickLeft};
        for (std::size_t index = 0; index < names.size(); ++index)
        {
            switch (frame.Buttons[index])
            {
            case ButtonChange::Press: handler.OnControllerButtonPressed(*names[index], device.User, device.Id, false); break;
            case ButtonChange::Repeat: handler.OnControllerButtonPressed(*names[index], device.User, device.Id, true); break;
            case ButtonChange::Release: handler.OnControllerButtonReleased(*names[index], device.User, device.Id, false); break;
            case ButtonChange::None: break;
            }
        }
        const std::array<const FName*, 6> axisNames{&FGamepadKeyNames::LeftAnalogX, &FGamepadKeyNames::LeftAnalogY, &FGamepadKeyNames::RightAnalogX, &FGamepadKeyNames::RightAnalogY, &FGamepadKeyNames::LeftTriggerAnalog, &FGamepadKeyNames::RightTriggerAnalog};
        for (std::size_t index = 0; index < frame.Axes.size(); ++index)
        {
            if (frame.SendAxes[index]) handler.OnControllerAnalog(*axisNames[index], device.User, device.Id, frame.Axes[index]);
        }
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
