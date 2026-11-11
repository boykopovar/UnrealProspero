[[vk::binding(0, 0)]] RWTexture2D<uint> outputTexture;
[[vk::binding(1, 0)]] RWStructuredBuffer<uint> outputValues;

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    outputTexture[uint2(id.x % 8, id.x / 8)] = 0x80000001u + id.x * 65537u;
    DeviceMemoryBarrierWithGroupSync();
    uint source = (id.x + 1) % 64;
    outputValues[id.x] = outputTexture[uint2(source % 8, source / 8)];
}
