[[vk::binding(0, 0)]] Texture2DArray<float> inputTexture;
[[vk::binding(1, 0)]] RWStructuredBuffer<uint> outputValues;

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint level = id.x % 4;
    uint width = 8u >> level;
    uint pixel = id.x / 4;
    outputValues[id.x] = uint(inputTexture.Load(int4(pixel % width, (pixel / width) % width, pixel % 3, level)));
}
