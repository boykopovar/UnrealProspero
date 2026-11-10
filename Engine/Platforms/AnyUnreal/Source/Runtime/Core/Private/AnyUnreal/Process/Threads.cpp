#include "AnyUnreal/AnyUnrealPlatformProcess.h"
#include "AnyUnreal/AnyUnrealRunnableThread.h"

FRunnableThread* FAnyUnrealPlatformProcess::CreateRunnableThread()
{
    return new FAnyUnrealRunnableThread();
}
