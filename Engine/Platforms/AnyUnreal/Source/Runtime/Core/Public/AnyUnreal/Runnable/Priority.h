#ifndef ANYUNREAL_RUNNABLE_PRIORITY_H
#define ANYUNREAL_RUNNABLE_PRIORITY_H

#include "GenericPlatform/GenericPlatformAffinity.h"

namespace AnyUnreal::Runnable
{
    int GetKernelPriority(EThreadPriority priority);
}

#endif
