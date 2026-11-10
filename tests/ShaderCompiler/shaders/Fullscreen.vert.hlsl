struct VertexOutput
{
    float4 position : SV_Position;
    float4 color : COLOR0;
};

VertexOutput main(uint vertexId : SV_VertexID)
{
    VertexOutput output;
    float2 position = float2((vertexId << 1) & 2, vertexId & 2);
    output.position = float4(position * float2(2, -2) + float2(-1, 1), 0, 1);
    output.color = float4(position, 0.5, 1);
    return output;
}
