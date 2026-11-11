#ifndef UNREALPROSPERO_RUNNABLE_PRIORITY_H
#define UNREALPROSPERO_RUNNABLE_PRIORITY_H

#include "GenericPlatform/GenericPlatformAffinity.h"

namespace UnrealProspero::Runnable
{
    int GetKernelPriority(EThreadPriority priority);
}

#endif
