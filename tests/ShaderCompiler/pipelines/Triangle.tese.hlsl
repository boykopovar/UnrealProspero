struct Vertex
{
    float4 position : SV_Position;
    float3 color : COLOR0;
};

struct Factors
{
    float edges[3] : SV_TessFactor;
    float inside : SV_InsideTessFactor;
};

[domain("tri")]
Vertex main(Factors factors, float3 coordinate : SV_DomainLocation, const OutputPatch<Vertex, 3> input)
{
    Vertex result;
    result.position = input[0].position * coordinate.x + input[1].position * coordinate.y + input[2].position * coordinate.z;
    result.color = input[0].color * coordinate.x + input[1].color * coordinate.y + input[2].color * coordinate.z;
    return result;
}
