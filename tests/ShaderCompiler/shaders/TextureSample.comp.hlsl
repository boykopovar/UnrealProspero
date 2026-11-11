[[vk::binding(0, 0)]] Texture2D<float> inputTexture;
[[vk::binding(1, 0)]] SamplerState textureSampler;
[[vk::binding(2, 0)]] RWStructuredBuffer<uint> outputValues;

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    float2 uv = float2((id.x % 8 + 0.75) / 8.0 + 1.0, (id.x / 8 + 0.5) / 8.0);
    outputValues[id.x] = uint(round(inputTexture.SampleLevel(textureSampler, uv, 0) * 4.0));
}
