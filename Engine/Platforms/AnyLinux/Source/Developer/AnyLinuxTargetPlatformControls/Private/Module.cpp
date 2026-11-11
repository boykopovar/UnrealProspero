#include "UnrealProspero/Cook/ControlsModule.h"

using FAnyLinuxTargetPlatformControlsModule = UnrealProspero::Cook::ControlsModule<EUnrealProsperoTarget::Linux>;

IMPLEMENT_MODULE(FAnyLinuxTargetPlatformControlsModule, AnyLinuxTargetPlatformControls)
