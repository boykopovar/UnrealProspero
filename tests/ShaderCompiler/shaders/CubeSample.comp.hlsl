[[vk::binding(0, 0)]] TextureCube<float> inputTexture;
[[vk::binding(1, 0)]] SamplerState textureSampler;
[[vk::binding(2, 0)]] RWStructuredBuffer<uint> outputValues;

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint face = id.x % 6;
    float3 direction = float3(0, 0, 0);
    direction[face / 2] = (face % 2) == 0 ? 1.0 : -1.0;
    outputValues[id.x] = uint(inputTexture.SampleLevel(textureSampler, direction, (id.x / 6) % 4));
}
