[[vk::binding(0, 0)]] Texture3D<float> inputTexture;
[[vk::binding(1, 0)]] RWStructuredBuffer<uint> outputValues;

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    outputValues[id.x] = uint(inputTexture.Load(int4(id.x % 5, (id.x / 5) % 3, id.x / 15, 0)));
}
