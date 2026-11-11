[[vk::binding(0, 0)]] Texture2D<int> inputTexture;
[[vk::binding(1, 0)]] RWStructuredBuffer<uint> outputValues;

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    outputValues[id.x] = uint(inputTexture.Load(int3(id.x % 8, id.x / 8, 0)) / 3);
}
