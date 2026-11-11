#ifndef ANYUNREAL_RHI_BUFFER_TYPED_FORMAT_H
#define ANYUNREAL_RHI_BUFFER_TYPED_FORMAT_H

#include "AnyUnreal/RHI/Descriptor/BufferDescriptor.h"
#include <PixelFormat.h>

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API BufferFormat ResolveTypedBufferFormat(EPixelFormat format);
    ANYUNREALRHI_API void ConfigureTypedBufferFormats();
}

#endif
