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

Factors Patch(InputPatch<Vertex, 3> input)
{
    Factors result;
    result.edges[0] = 1;
    result.edges[1] = 1;
    result.edges[2] = 1;
    result.inside = 1;
    return result;
}

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("Patch")]
Vertex main(InputPatch<Vertex, 3> input, uint id : SV_OutputControlPointID)
{
    return input[id];
}
