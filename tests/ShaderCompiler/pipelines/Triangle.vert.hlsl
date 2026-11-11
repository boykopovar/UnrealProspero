struct Vertex
{
    float4 position : SV_Position;
    float3 color : COLOR0;
};

Vertex main(uint id : SV_VertexID)
{
    const float2 positions[3] = {float2(-0.5, -0.5), float2(0.5, -0.5), float2(0.0, 0.5)};
    Vertex result;
    result.position = float4(positions[id], 0.0, 1.0);
    result.color = float3(id == 0, id == 1, id == 2);
    return result;
}
