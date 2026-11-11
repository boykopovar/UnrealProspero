[[vk::binding(2, 0)]] Texture2D<float4> textures[2];
[[vk::binding(7, 0)]] SamplerState textureSampler;
[[vk::binding(3, 1)]] cbuffer Constants
{
    float4 tint;
};

float4 main(float2 coordinates : TEXCOORD0) : SV_Target0
{
    return (textures[0].Sample(textureSampler, coordinates) + textures[1].Sample(textureSampler, coordinates)) * tint;
}
