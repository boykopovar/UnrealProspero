#ifndef UNREALPROSPERO_INPUT_CONTROLLER_EVENTS_H
#define UNREALPROSPERO_INPUT_CONTROLLER_EVENTS_H

#include "CoreMinimal.h"
#include "GenericPlatform/GenericApplicationMessageHandler.h"
#include "UnrealProspero/Input/ControllerHub.h"
#include "UnrealProspero/Input/ControllerState.h"
#include <array>
#include <map>

namespace UnrealProspero::Input
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
    };
}

#endif
