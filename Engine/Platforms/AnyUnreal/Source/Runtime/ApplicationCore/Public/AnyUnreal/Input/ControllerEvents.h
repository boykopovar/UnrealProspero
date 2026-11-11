#ifndef ANYUNREAL_INPUT_CONTROLLER_EVENTS_H
#define ANYUNREAL_INPUT_CONTROLLER_EVENTS_H

#include "CoreMinimal.h"
#include "GenericPlatform/GenericApplicationMessageHandler.h"
#include "AnyUnreal/Input/ControllerHub.h"
#include <array>
#include <map>

namespace AnyUnreal::Input
{
    class APPLICATIONCORE_API ControllerEvents final
    {
    public:
        explicit ControllerEvents(int initialUser);
        void Dispatch(const std::vector<ControllerSample>& samples, FGenericApplicationMessageHandler& handler, bool enabled);
        bool IsAttached() const;

    private:
        struct Device
        {
            FPlatformUserId User;
            FInputDeviceId Id;
            std::array<bool, 16> Buttons{};
            std::array<float, 6> Axes{};
            bool Connected = false;
        };
        void dispatchDevice(Device& device, const PadData* sample, FGenericApplicationMessageHandler& handler, bool enabled);
        std::map<int, Device> devices;
        int initialUser;
    };
}

#endif
