cbuffer VertexConstants : register(b0)
{
    float4 Scale;
};

float4 main(uint id : SV_VertexID) : SV_Position
{
    const float2 positions[3] = {float2(-0.5, -0.5), float2(0.5, -0.5), float2(0.0, 0.5)};
    return float4(positions[id] * Scale.xy, 0.0, 1.0);
}
