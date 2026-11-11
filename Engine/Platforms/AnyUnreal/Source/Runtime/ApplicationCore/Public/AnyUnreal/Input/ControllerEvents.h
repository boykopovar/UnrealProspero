#ifndef ANYUNREAL_INPUT_CONTROLLER_EVENTS_H
#define ANYUNREAL_INPUT_CONTROLLER_EVENTS_H

#include "CoreMinimal.h"
#include "GenericPlatform/GenericApplicationMessageHandler.h"
#include "AnyUnreal/Input/ControllerHub.h"
#include "AnyUnreal/Input/ControllerState.h"
#include <array>
#include <map>

namespace AnyUnreal::Input
{
    class APPLICATIONCORE_API ControllerEvents final
    {
    public:
        explicit ControllerEvents(int initialUser);
        void Dispatch(const std::vector<ControllerSample>& samples, FGenericApplicationMessageHandler& handler, bool enabled, double deltaSeconds);
        bool IsAttached() const;

    private:
        struct Device
        {
            FPlatformUserId User;
            FInputDeviceId Id;
            ControllerState State;
            bool Connected = false;
        };
        void dispatchDevice(Device& device, const PadData* sample, FGenericApplicationMessageHandler& handler, bool enabled, double deltaSeconds);
        std::map<int, Device> devices;
        int initialUser;
    };
}

#endif
