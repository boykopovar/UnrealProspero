#ifndef UNREALPROSPERO_RHI_BUFFER_TYPED_FORMAT_H
#define UNREALPROSPERO_RHI_BUFFER_TYPED_FORMAT_H

#include "UnrealProspero/RHI/Descriptor/BufferDescriptor.h"
#include <PixelFormat.h>

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API BufferFormat ResolveTypedBufferFormat(EPixelFormat format);
    UNREALPROSPERORHI_API void ConfigureTypedBufferFormats();
}

#endif
