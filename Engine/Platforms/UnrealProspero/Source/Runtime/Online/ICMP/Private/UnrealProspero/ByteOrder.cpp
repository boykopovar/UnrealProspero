#include "Misc/ByteSwap.h"

uint16 NtoHS(uint16 value) { return ByteSwap(value); }
uint16 HtoNS(uint16 value) { return ByteSwap(value); }
uint32 NtoHL(uint32 value) { return ByteSwap(value); }
uint32 HtoNL(uint32 value) { return ByteSwap(value); }
