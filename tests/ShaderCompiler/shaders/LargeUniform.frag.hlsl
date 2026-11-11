[[vk::binding(0, 0)]] cbuffer Constants
{
    float4 values[4096];
};

float4 main(float2 coordinates : TEXCOORD0) : SV_Target0
{
    return values[min(uint(coordinates.x * 4096), 4095)];
}
