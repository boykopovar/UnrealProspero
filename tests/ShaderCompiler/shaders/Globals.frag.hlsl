float4 tint;
float intensity;

float4 main(float2 coordinates : TEXCOORD0) : SV_Target0
{
    return tint * intensity * coordinates.x;
}
