[[vk::binding(0, 0)]] RWTexture3D<float> outputTexture;
[[vk::binding(1, 0)]] RWStructuredBuffer<uint> outputValues;

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    outputTexture[uint3(id.x % 5, (id.x / 5) % 3, id.x / 15)] = float(id.x * 3 + 1);
    DeviceMemoryBarrierWithGroupSync();
    uint source = (id.x + 1) % 64;
    outputValues[id.x] = uint(outputTexture[uint3(source % 5, (source / 5) % 3, source / 15)]);
}
